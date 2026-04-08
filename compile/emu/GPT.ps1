param (
	[string]$JsonFile,
	[string]$OutputPath,
	[bool]$IsDrive,
	[switch]$UseMBR,
	[string]$BootBinary,
	[string]$LogFile,
	[switch]$Verbose,
	[switch]$Help,
	[switch]$OutputJSON
)

# Help Information
function Show-Help {
	$helpText = @"
Smart GPT/MBR Disk Emulator

SYNOPSIS
	GPT.ps1 [Parameters]

DESCRIPTION
	Creates GPT or MBR formatted disk images or writes to physical drives with intelligent
	validation, space checking, and optional boot code embedding.

PARAMETERS
	-JsonFile <string>
		Path to JSON configuration file containing partition definitions.
		Required format: 
		{
			"partitions": [
				{
					"name": "partition_name",
					"size": <bytes>,
					"type": "<GUID>",
					"attributes": "attribute1" | ["attribute1", "attribute2"]
				}, ...
			]
		}
		
		Supported partition attributes:
		- "system"          : Required for platform to function (bit 0)
		- "firmware-ignore" : Firmware should ignore this partition (bit 1)
		- "legacy-boot"     : Legacy BIOS bootable (bit 2)
		- "efi-boot"        : EFI/boot partition (system attribute)
		- "hidden"          : Hidden partition (bit 49)
		- "readonly"        : Read-only partition (bit 48)
		- "required"        : Required for OS (bit 0)
		- "0xHEXVALUE"      : Custom hex attribute value
		
		Attributes can be a single string or an array of strings (combined with bitwise OR)

	-OutputPath <string>
		Output file path for disk image or drive letter for physical drive (e.g., 'D:')

	-IsDrive <bool>
		If $true, writes to physical drive. If $false, creates image file.

	-UseMBR [switch]
		Create MBR partition table instead of GPT (max 4 partitions)

	-BootBinary <string>
		Path to binary file to embed in boot code area (max 446 bytes)

	-LogFile <string>
		Path to output log file for all operations

	-Verbose [switch]
		Enable verbose output with detailed operation information

	-Help [switch]
		Display this help information and exit

	-OutputJSON [switch]
		Output partition configuration as JSON and exit (does not create image)

PARTITION POST-PROCESSING
	The "give" field in partition definitions supports array of post-processing directives:
	
	- "fit"   : If partition causes overflow, shrink to fit remaining available space
	- "grow"  : Expand partition to fill all remaining available space (only one partition)
	
	Examples:
		"give": "fit"                  # Single directive as string
		"give": ["fit"]                # Array with fit directive
		"give": ["grow"]               # Expand to fill space
		"give": ["fit", "grow"]        # First fit if needed, then grow (exclusive)

EXAMPLES
	# Display help
	.\GPT.ps1 -Help

	# Create GPT disk image from config
	.\GPT.ps1 -JsonFile config.json -OutputPath disk.img -LogFile operations.log -Verbose

	# Output configuration as JSON
	.\GPT.ps1 -JsonFile config.json -OutputJSON

	# Create MBR with boot code
	.\GPT.ps1 -JsonFile config.json -OutputPath mbr.img -UseMBR -BootBinary bootloader.bin

	# Write to physical drive with confirmation
	.\GPT.ps1 -JsonFile config.json -OutputPath D: -IsDrive \$true

JSON CONFIG EXAMPLE
	{
		"partitions": [
			{
				"name": "EFI System",
				"size": 1073741824,
				"type": "C12A7328-F81F-11D2-BA4B-00A0C93EC93B",
				"attributes": "efi-boot",
				"give": "fit"
			},
			{
				"name": "Data Partition",
				"size": 5368709120,
				"type": "0FC63DAF-8483-4772-8E79-3D69D8477DE4",
				"attributes": ["system", "required"],
				"give": ["grow"]
			},
			{
				"name": "Hidden Data",
				"size": 2147483648,
				"type": "0FC63DAF-8483-4772-8E79-3D69D8477DE4",
				"attributes": "hidden"
			}
		]
	}
"@
	Write-Host $helpText
}

# Exit if Help flag is set
if ($Help) {
	Show-Help
	exit 0
}

# CRC32 Calculation Function
function Calculate-CRC32{
	param([byte[]]$Data)
	$crc32Table = @()
	for($i = 0; $i -lt 256; $i++){
		$crc = $i
		for($j = 0; $j -lt 8; $j++){
			if($crc -band 0x80000000){
				$crc = ($crc -shl 1) -bxor 0x04C11DB7
			}else{$crc = $crc -shl 1}
			$crc = $crc -band 0xFFFFFFFF
		}
		$crc32Table += $crc
	}

	$crc = 0xFFFFFFFF
	foreach($byte in $Data){
		$tableIndex = ($crc -bxor $byte) -band 0xFF
		$crc = ($crc -shr 8) -bxor $crc32Table[$tableIndex]
		$crc = $crc -band 0xFFFFFFFF
	}
	return ($crc -bxor 0xFFFFFFFF) -band 0xFFFFFFFF
}

# Function to create GPT Header
function Create-GPTHeader{
	param(
		[uint64]$DiskSize,
		[byte[]]$DiskGUID,
		[uint32]$PartitionCount = 128
	)

	$headerSize = 92
	$entrySize = 128
	$entriesPerLBA = 512 / $entrySize
	$entriesLBA = 2
	$backupLBA = $DiskSize / 512 - 1
	$lastUsableLBA = $backupLBA - 33

	$header = New-Object System.IO.MemoryStream
	$writer = New-Object System.IO.BinaryWriter($header)

	$writer.Write([byte[]]@(0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54), 0, 8)
	$writer.Write([uint32]0x00010000)
	$writer.Write([uint32]$headerSize)
	$writer.Write([uint32]0)  # CRC placeholder
	$writer.Write([uint32]0)
	$writer.Write([uint64]1)
	$writer.Write([uint64]$backupLBA)
	$writer.Write([uint64]34)  # First usable LBA
	$writer.Write([uint64]$lastUsableLBA)
	$writer.Write($DiskGUID)
	$writer.Write([uint64]$entriesLBA)
	$writer.Write([uint32]$PartitionCount)
	$writer.Write([uint32]$entrySize)
	$writer.Write([uint32]0)  # Entries CRC placeholder

	$headerBytes = $header.ToArray()

	return $headerBytes, @{
		BackupLBA = $backupLBA
		EntriesLBA = $entriesLBA
		LastUsableLBA = $lastUsableLBA
		HeaderSize = $headerSize
	}
}

# Function to create Partition Entry
function Create-PartitionEntry{
	param(
		[guid]$TypeGUID,
		[guid]$UniqueGUID,
		[uint64]$StartLBA,
		[uint64]$EndLBA,
		[string]$PartitionName,
		[uint64]$Attributes = 0
	)

	$entry = New-Object System.IO.MemoryStream
	$writer = New-Object System.IO.BinaryWriter($entry)

	$writer.Write($TypeGUID.ToByteArray())
	$writer.Write($UniqueGUID.ToByteArray())
	$writer.Write([uint64]$StartLBA)
	$writer.Write([uint64]$EndLBA)
	$writer.Write([uint64]$Attributes)

	$nameBytes = [System.Text.Encoding]::Unicode.GetBytes($PartitionName)
	$writer.Write($nameBytes, 0, [Math]::Min($nameBytes.Length, 72))
	if($nameBytes.Length -lt 72){$writer.Write((New-Object byte[] (72 - $nameBytes.Length)))}
	return $entry.ToArray()
}

# Function to parse partition attributes
function Parse-PartitionAttributes {
	param([string[]]$AttributeInput)

	$attributes = 0

	# Predefined attribute presets
	$presets = @{
		"system"           = 0x0000000000000001  # Bit 0: Required for platform to function
		"firmware-ignore"  = 0x0000000000000002  # Bit 1: Firmware should ignore this partition
		"legacy-boot"      = 0x0000000000000004  # Bit 2: Legacy BIOS bootable
		"efi-boot"         = 0x0000000000000001  # System/EFI boot partition
		"hidden"           = 0x0000000002000000  # Bit 49: Hidden partition (vendor specific)
		"readonly"         = 0x0000000001000000  # Bit 48: Read-only partition (vendor specific)
		"required"         = 0x0000000000000001
	}

	if ($null -eq $AttributeInput){return $attributes}

	foreach ($attr in $AttributeInput) {
		if($attr -match '^0x[0-9A-Fa-f]+$'){
			# Hex value
			$attributes = $attributes -bor [uint64]::Parse($attr, [System.Globalization.NumberStyles]::HexNumber)
		}elseif($presets.ContainsKey($attr.ToLower())){
			# Preset name
			$attributes = $attributes -bor $presets[$attr.ToLower()]
		}else{Write-Warning "Unknown partition attribute: $attr"}
	}
	return $attributes
}

# Function to parse partition post-processing directives
function Parse-PartitionDirectives {
	param([object]$GiveInput)

	$directives = @()
	if ($null -eq $GiveInput) { return $directives }

	if ($GiveInput -is [string]) {
		$directives += @($GiveInput)
	} elseif ($GiveInput -is [array]) {
		$directives += @($GiveInput)
	}

	return $directives
}

# Function to apply partition post-processing
function Apply-PartitionPostProcessing {
	param(
		[PSObject[]]$Partitions,
		[uint64]$AvailableSpace,
		[uint64]$SectorSize
	)

	$sectorSize = 512
	$processed = @()
	$growTargetIndex = -1

	# First pass: identify grow directive
	for ($i = 0; $i -lt $Partitions.Count; $i++) {
		$directives = Parse-PartitionDirectives -GiveInput $Partitions[$i].give
		if ($directives -contains "grow") {
			$growTargetIndex = $i
			break
		}
	}

	# Apply directives
	$totalNeeded = 0
	for ($i = 0; $i -lt $Partitions.Count; $i++) {
		$partition = $Partitions[$i]
		$directives = Parse-PartitionDirectives -GiveInput $partition.give
		$partSize = [uint64]$partition.size

		if ($i -eq $growTargetIndex -and $directives -contains "grow") {
			# Skip grow partition in this pass, handle after fit
			$processed += @{ index = $i; size = $partSize; directives = $directives }
			continue
		}

		if ($directives -contains "fit" -and ($totalNeeded + $partSize) -gt $AvailableSpace) {
			# Shrink to fit
			$partSize = $AvailableSpace - $totalNeeded
			Write-Host "Applying 'fit' to partition '$($partition.name)': shrinking to $partSize bytes"
		}

		$processed += @{ index = $i; size = $partSize; directives = $directives }
		$totalNeeded += $partSize
	}

	# Apply grow directive if present
	if ($growTargetIndex -ge 0) {
		$remainingSpace = $AvailableSpace - $totalNeeded
		if ($remainingSpace -gt 0) {
			$remainingSpace = [Math]::Floor($remainingSpace / $sectorSize) * $sectorSize
			$processed[$growTargetIndex].size += $remainingSpace
			Write-Host "Applying 'grow' to partition '$($Partitions[$growTargetIndex].name)': adding $remainingSpace bytes"
		}
	}

	return $processed
}

# Function to create Protective MBR
function Create-ProtectiveMBR{
	param([uint64]$MaxLBA, [byte[]]$BootCode)

	$mbr = New-Object byte[] 512
	$writer = New-Object System.IO.BinaryWriter([System.IO.MemoryStream]::new($mbr))

	# Boot code
	if($BootCode){$writer.Write($BootCode, 0, [Math]::Min($BootCode.Length, 446))}
	$writer.BaseStream.Seek(446, [System.IO.SeekOrigin]::Begin)

	$writer.Write([byte]0)
	$writer.Write([byte]0)
	$writer.Write([byte]0)
	$writer.Write([byte]0)
	$writer.Write([byte]0xEE)
	$writer.Write([byte]0xFF)
	$writer.Write([byte]0xFF)
	$writer.Write([byte]0xFF)
	$writer.Write([uint32]1)
	$writer.Write([uint32]$MaxLBA)

	$writer.BaseStream.Seek(510, [System.IO.SeekOrigin]::Begin)
	$writer.Write([byte]0x55)
	$writer.Write([byte]0xAA)

	return $mbr
}

# Function to create MBR Partition Entry
function Create-MBRPartitionEntry{
	param(
		[byte]$BootFlag,
		[byte]$Type,
		[uint32]$StartLBA,
		[uint32]$Sectors
	)

	$entry = New-Object byte[] 16
	$writer = New-Object System.IO.BinaryWriter([System.IO.MemoryStream]::new($entry))

	$writer.Write($BootFlag)
	$writer.Write([byte]0) # CHS start
	$writer.Write([byte]0)
	$writer.Write([byte]0)
	$writer.Write($Type)
	$writer.Write([byte]0) # CHS end
	$writer.Write([byte]0)
	$writer.Write([byte]0)
	$writer.Write([uint32]$StartLBA)
	$writer.Write([uint32]$Sectors)

	return $entry
}

# Function to create MBR
function Create-MBR{
	param([uint64]$MaxLBA, [byte[]]$BootCode, [byte[][]]$PartitionEntries)

	$mbr = New-Object byte[] 512
	$writer = New-Object System.IO.BinaryWriter([System.IO.MemoryStream]::new($mbr))

	# Boot code
	if($BootCode){$writer.Write($BootCode, 0, [Math]::Min($BootCode.Length, 446))}

	# Partition entries
	$writer.BaseStream.Seek(446, [System.IO.SeekOrigin]::Begin)
	foreach($entry in $PartitionEntries){$writer.Write($entry)}

	# Signature
	$writer.BaseStream.Seek(510, [System.IO.SeekOrigin]::Begin)
	$writer.Write([byte]0x55)
	$writer.Write([byte]0xAA)

	return $mbr
}

# Parse JSON file
$json = Get-Content -Path $JsonFile -Raw | ConvertFrom-Json

# If OutputJSON flag is set, output partition info as JSON and exit
if($OutputJSON){
	$outputInfo = @{
		diskType = if ($UseMBR) { "MBR" } else { "GPT" }
		bootBinary = if ($BootBinary) { (Split-Path -Leaf $BootBinary) } else { $null }
		partitions = @()
	}

	$sectorSize = 512
	$gptOverhead = 34 * $sectorSize
	$totalPartitionSize = 0
	$currentLBA = 34

	# Calculate initial sizes
	foreach($partition in $json.partitions){
		$totalPartitionSize += [uint64]$partition.size
	}
	$diskSize = $totalPartitionSize + $gptOverhead
	$diskSize = [Math]::Ceiling($diskSize / $sectorSize) * $sectorSize
	
	# Apply post-processing
	$availableSpace = $diskSize - $gptOverhead
	$processedPartitions = Apply-PartitionPostProcessing -Partitions $json.partitions -AvailableSpace $availableSpace -SectorSize $sectorSize

	foreach($proc in $processedPartitions){
		$partition = $json.partitions[$proc.index]
		$size = [uint64]$proc.size
		$sectors = [Math]::Ceiling($size / $sectorSize)
		$startLBA = $currentLBA
		$endLBA = $startLBA + $sectors - 1

		$partAttributes = 0
		if ($partition.attributes) {
			if($partition.attributes -is [string]){
				$partAttributes = Parse-PartitionAttributes -AttributeInput @($partition.attributes)
			}elseif($partition.attributes -is [array]){
				$partAttributes = Parse-PartitionAttributes -AttributeInput $partition.attributes
			}
		}

		$outputInfo.partitions += @{
			name = $partition.name
			size = $size
			sizeBytes = $size
			sectors = $sectors
			startLBA = $startLBA
			endLBA = $endLBA
			type = $partition.type
			attributes = @{
				raw = "0x" + $partAttributes.ToString("X16")
				decimal = $partAttributes
				flags = if ($partition.attributes) { $partition.attributes } else { @() }
			}
			directives = $proc.directives
		}

		$currentLBA = $endLBA + 1
	}

	$finalDiskSize = $totalPartitionSize + $gptOverhead
	$finalDiskSize = [Math]::Ceiling($finalDiskSize / $sectorSize) * $sectorSize

	$outputInfo | Add-Member -NotePropertyName "totalDiskSize" -NotePropertyValue $finalDiskSize
	$outputInfo | Add-Member -NotePropertyName "totalDiskSizeGB" -NotePropertyValue ([Math]::Round($finalDiskSize / 1GB, 2))
	$outputInfo | Add-Member -NotePropertyName "gptProvenance" -NotePropertyValue @{
		mbrLBA = 0
		primaryHeaderLBA = 1
		partitionEntriesStartLBA = 2
		dataStartLBA = 34
		backupEntriesLBA = $([Math]::Ceiling($finalDiskSize / $sectorSize) - 33)
		backupHeaderLBA = $([Math]::Ceiling($finalDiskSize / $sectorSize) - 1)
	}

	$outputInfo | ConvertTo-Json -Depth 10
	exit 0
}
$sectorSize = 512
$gptOverhead = 34 * $sectorSize  # MBR + Header + Entries + Backup
$totalPartitionSize = 0
foreach($partition in $json.partitions){
	$totalPartitionSize += [uint64]$partition.size
}
$diskSize = $totalPartitionSize + $gptOverhead
$diskSize = [Math]::Ceiling($diskSize / $sectorSize) * $sectorSize  # Align to sector

Write-Host "Total Disk Size: $($diskSize) bytes"

# Apply partition post-processing directives
$availableSpace = $diskSize - $gptOverhead
$processedPartitions = Apply-PartitionPostProcessing -Partitions $json.partitions -AvailableSpace $availableSpace -SectorSize $sectorSize

# Recalculate disk size if partitions were modified
$newTotalSize = 0
foreach ($proc in $processedPartitions) {
	$newTotalSize += $proc.size
}
$newDiskSize = $newTotalSize + $gptOverhead
$newDiskSize = [Math]::Ceiling($newDiskSize / $sectorSize) * $sectorSize

if ($newDiskSize -ne $diskSize) {
	Write-Host "Disk size adjusted from $diskSize to $newDiskSize bytes due to partition post-processing"
	$diskSize = $newDiskSize
}

# Generate Disk GUID
$diskGUID = [guid]::NewGuid()

# Create partition entries with processed sizes
$partitionEntries = @()
$currentLBA = 34  # After GPT areas
for ($i = 0; $i -lt $processedPartitions.Count; $i++) {
	$proc = $processedPartitions[$i]
	$partition = $json.partitions[$proc.index]
	$size = [uint64]$proc.size
	$sectors = [Math]::Ceiling($size / $sectorSize)
	$startLBA = $currentLBA
	$endLBA = $startLBA + $sectors - 1
	$typeGUID = [guid]$partition.type
	$uniqueGUID = [guid]::NewGuid()
	
	# Parse partition attributes
	$partAttributes = 0
	if($partition.attributes){
		if($partition.attributes -is [string]){
			$partAttributes = Parse-PartitionAttributes -AttributeInput @($partition.attributes)
		}elseif($partition.attributes -is [array]){
			$partAttributes = Parse-PartitionAttributes -AttributeInput $partition.attributes
		}
	}
	
	$entry = Create-PartitionEntry -TypeGUID $typeGUID -UniqueGUID $uniqueGUID -StartLBA $startLBA -EndLBA $endLBA -PartitionName $partition.name -Attributes $partAttributes
	$partitionEntries += $entry
	$currentLBA = $endLBA + 1
}

# Pad entries to 128
while($partitionEntries.Count -lt 128){$partitionEntries += (New-Object byte[] 128)}

# Create GPT Header
$headerBytes, $headerInfo = Create-GPTHeader -DiskSize $diskSize -DiskGUID $diskGUID.ToByteArray()

# Calculate entries CRC
$entriesBytes = [byte[]]::new(0)
foreach ($entry in $partitionEntries) {
	$entriesBytes += $entry
}
$entriesCRC = Calculate-CRC32 -Data $entriesBytes
[Array]::Copy([BitConverter]::GetBytes($entriesCRC), 0, $headerBytes, 88, 4)

# Calculate header CRC
$headerCRC = Calculate-CRC32 -Data $headerBytes
[Array]::Copy([BitConverter]::GetBytes($headerCRC), 0, $headerBytes, 16, 4)

# Create Protective MBR
$mbrBytes = Create-ProtectiveMBR -MaxLBA ($diskSize / $sectorSize - 1)

# Create Backup Header (similar to primary but swapped LBAs)
$backupHeader = $headerBytes.Clone()
[Array]::Copy([BitConverter]::GetBytes($headerInfo.BackupLBA), 0, $backupHeader, 24, 8)
[Array]::Copy([BitConverter]::GetBytes(1), 0, $backupHeader, 32, 8)
[Array]::Copy([BitConverter]::GetBytes($headerInfo.BackupLBA - 32), 0, $backupHeader, 72, 8)
$backupCRC = Calculate-CRC32 -Data $backupHeader
[Array]::Copy([BitConverter]::GetBytes($backupCRC), 0, $backupHeader, 16, 4)

if(-not $IsDrive){
	# Work on File
	if(Test-Path -Path $OutputPath -PathType Container){
		$imagePath = Join-Path -Path $OutputPath -ChildPath "disk.img"
	}else{$imagePath = $OutputPath}

	# Check space
	$driveLetter = Split-Path -Path $imagePath -Qualifier
	$drive = Get-PSDrive -Name $driveLetter.TrimEnd(':')
	if($drive.Free -lt $diskSize){
		Write-Error "Not enough space on drive $driveLetter. Required: $diskSize bytes, Available: $($drive.Free) bytes"
		exit 1
	}

	# Check if file exists and is up to date (simple check: size)
	$needsUpdate = $true
	if(Test-Path -Path $imagePath){
		$fileInfo = Get-Item -Path $imagePath
		if($fileInfo.Length -eq $diskSize){
			Write-Host "Image file already exists and is the correct size. Skipping update."
			$needsUpdate = $false
		}
	}

	if($needsUpdate){
		Write-Host "Creating GPT disk image at $imagePath"
		$fs = [System.IO.File]::Create($imagePath)
		try {
			# Write MBR
			$fs.Write($mbrBytes, 0, $mbrBytes.Length)

			# Write Primary Header
			$fs.Write($headerBytes, 0, $headerBytes.Length)

			# Write Partition Entries
			foreach($entry in $partitionEntries){$fs.Write($entry, 0, $entry.Length)}

			# Seek to data area and write zeros or data (for now zeros)
			$fs.Seek($gptOverhead, [System.IO.SeekOrigin]::Begin)
			$fs.Write((New-Object byte[] ($diskSize - $gptOverhead)), 0, $diskSize - $gptOverhead)

			# Write Backup Entries (empty)
			$fs.Seek($headerInfo.BackupLBA * $sectorSize - 33 * $sectorSize, [System.IO.SeekOrigin]::Begin)
			for($i = 0; $i -lt 32; $i++){$fs.Write((New-Object byte[] $sectorSize), 0, $sectorSize)}

			# Write Backup Header
			$fs.Seek($headerInfo.BackupLBA * $sectorSize, [System.IO.SeekOrigin]::Begin)
			$fs.Write($backupHeader, 0, $backupHeader.Length)

		}finally{$fs.Close()}
		Write-Host "GPT disk image created successfully."
	}
}else{
	# Work on Drive
	$driveLetter = $OutputPath.TrimEnd(':')

	# Check space
	$drive = Get-PSDrive -Name $driveLetter
	if ($drive.Free -lt $diskSize) {
		Write-Error "Not enough space on drive $driveLetter. Required: $diskSize bytes, Available: $($drive.Free) bytes"
		exit 1
	}

	# Ask for confirmation
	$confirm = Read-Host "WARNING: This will write GPT structure to drive $driveLetter, potentially destroying all data. Confirm? (y/N)"
	if ($confirm -ne 'y' -and $confirm -ne 'Y') {
		Write-Host "Operation cancelled."
		exit 0
	}

	# Unmount if mounted
	try{
		Dismount-DiskImage -DriveLetter $driveLetter -ErrorAction Stop
		Write-Host "Drive $driveLetter unmounted."
	}catch{Write-Host "Drive $driveLetter was not mounted or could not be unmounted."}

	# Write to drive
	Write-Host "Writing GPT structure to drive $driveLetter"
	$drivePath = "\\.\$driveLetter`:"
	$fs = [System.IO.File]::Open($drivePath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write)
	try{
		# Write MBR
		$fs.Write($mbrBytes, 0, $mbrBytes.Length)

		# Write Primary Header
		$fs.Write($headerBytes, 0, $headerBytes.Length)

		# Write Partition Entries
		foreach($entry in $partitionEntries){$fs.Write($entry, 0, $entry.Length)}

		# Note: Not overwriting data area, assuming partitions are to be created separately

	} finally {
		$fs.Close()
	}
	Write-Host "GPT structure written to drive successfully."
}