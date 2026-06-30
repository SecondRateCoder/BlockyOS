param(
	[string]$DiskImage,
	[string]$PartitionName,
	[string]$PartitionFlag,
	[string]$FileSystemType = "FAT32",
	[string]$VolumeName = "BlockyOS",
	[byte[]]$BootCode = $null,
	[string]$SourceDirectory,
	[string]$LogFile,
	[switch]$Verbose,
	[switch]$Help
)

function New-LoggedFileStream{
	param(
		[string]$Path
	)

	$proxy = New-Object PSObject -Property @{
		BaseStream = [System.IO.File]::Open($Path, [System.IO.FileMode]::OpenOrCreate, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::Read)
		LastSeek   = 0
	}

	$proxy | Add-Member -MemberType ScriptMethod -Name Seek -Value {
		param($offset, $origin)

		$this.LastSeek = $offset
		Log-Warning ">> [SEEK] Offset=$offset Origin=$origin  (Caller: $((Get-PSCallStack)[1].Location))"

		return $this.BaseStream.Seek($offset, $origin)
	}

	$proxy | Add-Member -MemberType ScriptMethod -Name Write -Value {
		param([byte[]]$buffer, $offset, $count)

		if(($count -eq 0) -or (-not $count)){$count = $buffer.Count - $offset}
		$caller = (Get-PSCallStack)[1].Location
		$abs = $this.BaseStream.Position

		Log-Warning ">> [WRITE] (Position=$abs  Count=$count  Caller=$caller)"

		return $this.BaseStream.Write($buffer, $offset, $count)
	}

	return $proxy
}

function Show-Help {
@"
Partition File System Driver (FAT16 / FAT32 / NTFS)

SYNOPSIS
	Formats a single partition inside an existing disk image (GPT),
	selected by partition name and attribute flag, and optionally
	loads a directory tree into it (FAT16/FAT32).

USAGE
	.\FSDriver.ps1 -DiskImage disk.img -PartitionName "EFI" -PartitionFlag "efi-boot" -FileSystemType FAT32 -SourceDirectory C:\Data -LogFile fs.log -Verbose

PARAMETERS
	-DiskImage       Path to existing disk image (with GPT)
	-PartitionName   GPT partition name to match
	-PartitionFlag   Attribute flag to match (e.g. "system", "efi-boot", "hidden")
	-FileSystemType  FAT16, FAT32, or NTFS (default FAT32)
	-SourceDirectory Directory to load into partition (FAT16/FAT32)
	-LogFile         Path to log file
	-Verbose         Verbose console output
	-Help            Show this help
"@
}

if ($Help) { Show-Help; exit }

$logBuffer = @()

function Log-Message{
	param(
		[string]$Message,
		[string]$Level = "INFO"
	)

	$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
	$logEntry = "[$timestamp] [$Level] $Message"
	$logBuffer += $logEntry

	if($Verbose){Write-Host $logEntry}

	if($LogFile){Add-Content -Path $LogFile -Value $logEntry -ErrorAction SilentlyContinue}
}

function Log-Warning{
	param([string]$Message)
	Log-Message -Message $Message -Level "WARNING"
	Write-Warning $Message
}

function Log-Error{
	param([string]$Message)
	Log-Message -Message $Message -Level "ERROR"
	Write-Error $Message
}

if ($LogFile) {
	try {
		$logPath = Split-Path -Path $LogFile
		if ($logPath -and -not (Test-Path -Path $logPath)) {
			New-Item -ItemType Directory -Path $logPath -Force | Out-Null
		}
		"=== Partition FS Driver Session ===" | Out-File -FilePath $LogFile -Force
	} catch {
		Write-Warning "Could not initialize log file: $_"
	}
}

Log-Message "Partition FS Driver Started"

if (-not $DiskImage) {
	Log-Error "DiskImage parameter is required"
	exit 1
}

if (-not (Test-Path -Path $DiskImage -PathType Leaf)) {
	Log-Error "Disk image not found: $DiskImage"
	exit 1
}

if (-not $PartitionName) {
	Log-Error "PartitionName parameter is required"
	exit 1
}

if (-not $PartitionFlag) {
	Log-Error "PartitionFlag parameter is required"
	exit 1
}

$validFS = @("FAT16","FAT32","NTFS")
if ($FileSystemType -notin $validFS) {
	Log-Error "Invalid FileSystemType: $FileSystemType. Must be one of: $($validFS -join ', ')"
	exit 1
}

function Get-GptAttributeMask {
	param([string]$Flag)

	switch ($Flag) {
		"system"         { return [uint64]0x0000000000000001 }
		"required"       { return [uint64]0x0000000000000001 }
		"efi-boot"       { return [uint64]0x0000000000000001 }
		"firmware-ignore"{ return [uint64]0x0000000000000002 }
		"legacy-boot"    { return [uint64]0x0000000000000004 }
		"readonly"       { return [uint64]0x1000000000000000 }
		"hidden"         { return [uint64]0x8000000000000000 }
		default {
			if ($Flag -match "^0x[0-9A-Fa-f]+$") {
				return [UInt64]::Parse($Flag.Substring(2), "HexNumber")
			} else {
				Log-Error "Unknown PartitionFlag '$Flag'. Use known names or 0xHEXVALUE."
				exit 1
			}
		}
	}
}

$flagMask = Get-GptAttributeMask -Flag $PartitionFlag

$sectorSize = 512

try {
	$fs = New-LoggedFileStream $DiskImage
	$br = New-Object System.IO.BinaryReader($fs.BaseStream)
} catch {
	Log-Error "Could not open disk image: $_"
	exit 1
}

function Read-Sector {
	param([UInt64]$LBA)

	$fs.Seek([int64]($LBA * $sectorSize), [System.IO.SeekOrigin]::Begin) | Out-Null
	return $br.ReadBytes($sectorSize)
}

$gptHeader = Read-Sector 1
$signature = [Text.Encoding]::ASCII.GetString($gptHeader,0,8)

if ($signature -ne "EFI PART") {
	Log-Error "Disk image does not contain a valid GPT header at LBA 1 (signature: '$signature')"
	$br.Close()
	$fs.BaseStream.Close()
	exit 1
}

$entriesLBA   = [BitConverter]::ToUInt64($gptHeader,72)
$entryCount   = [BitConverter]::ToUInt32($gptHeader,80)
$entrySize    = [BitConverter]::ToUInt32($gptHeader,84)

Log-Message "GPT detected: EntriesLBA=$entriesLBA, EntryCount=$entryCount, EntrySize=$entrySize"

$selectedPartition = $null

for ($i = 0; $i -lt $entryCount; $i++) {
	$entryOffsetBytes = ($entriesLBA * $sectorSize) + ($i * $entrySize)
	$fs.Seek([int64]$entryOffsetBytes, [System.IO.SeekOrigin]::Begin) | Out-Null
	$entry = $br.ReadBytes($entrySize)

	$typeGuidBytes = $entry[0..15]
	$allZero = $true
	foreach($b in $typeGuidBytes){
		if($b -ne 0){$allZero = $false; break}
	}
	if($allZero){continue}

	$attr = [BitConverter]::ToUInt64($entry,48)

	$nameBytes = $entry[56..($entrySize-1)]
	$name = [Text.Encoding]::Unicode.GetString($nameBytes).TrimEnd([char]0)

	if($name -ne $PartitionName){continue}
	if(($attr -band $flagMask) -eq 0){continue}

	$startLBA = [BitConverter]::ToUInt64($entry,32)
	$endLBA   = [BitConverter]::ToUInt64($entry,40)

	$selectedPartition = [PSCustomObject]@{
		Index    = $i
		Name     = $name
		Attr     = $attr
		StartLBA = $startLBA
		EndLBA   = $endLBA
	}
	break
}

if (-not $selectedPartition) {
	Log-Error "No partition found with Name='$PartitionName' and Flag='$PartitionFlag'"
	$br.Close()
	$fs.BaseStream.Close()
	exit 1
}

$partitionSizeSectors = $selectedPartition.EndLBA - $selectedPartition.StartLBA + 1
$partitionSizeBytes   = $partitionSizeSectors * $sectorSize
$partitionOffsetBytes = $selectedPartition.StartLBA * $sectorSize

Log-Message "Selected partition:"
Log-Message "  Index   : $($selectedPartition.Index)"
Log-Message "  Name    : $($selectedPartition.Name)"
Log-Message "  Attr    : 0x$('{0:X16}' -f $selectedPartition.Attr)"
Log-Message "  StartLBA: $($selectedPartition.StartLBA)"
Log-Message "  EndLBA  : $($selectedPartition.EndLBA)"
Log-Message "  Size    : $partitionSizeBytes bytes"

# This function will now generate the correct
function Create-FAT32BootSector {
    param(
        [uint64]$PartitionSizeBytes,
        [string]$VolumeLabel = "BLOCKYOS",
        [uint32]$ClusterSize = 4096,
        [byte[]]$BootCode
    )

    $sectorSize = 512
    $sectorsPerCluster = [byte]($ClusterSize / $sectorSize)
    $reservedSectors = 32
    $fatCopies = 2
    $totalSectors = [uint32]($PartitionSizeBytes / $sectorSize)

    # Compute FAT size iteratively
    $fatSizeInSectors = 0
    $previousFatSize = -1
    do {
        $previousFatSize = $fatSizeInSectors
        $dataSectors = $totalSectors - $reservedSectors - ($fatCopies * $fatSizeInSectors)
        $clusterCount = [uint32][Math]::Floor($dataSectors / $sectorsPerCluster)
        $fatSizeInSectors = [uint32][Math]::Ceiling((($clusterCount + 2) * 4.0) / $sectorSize)
    } while ($fatSizeInSectors -ne $previousFatSize)

	$bootbinaries = $null
    if($BootCode){$bootbinaries = $BootCode
    }else{$bootbinaries = New-Object byte[] 512}
    $ms = New-Object System.IO.MemoryStream($bootbinaries, 0, $bootbinaries.Length, $true)
    $writer = New-Object System.IO.BinaryWriter($ms)
    $reader = New-Object System.IO.BinaryReader($ms)

    # Helper to read/write at offsets
    function Write-At($offset, [ScriptBlock]$action) {
        $ms.Position = $offset
        & $action
    }
    function ReadUInt16-At($offset) {
        $ms.Position = $offset
        return $reader.ReadUInt16()
    }
    function ReadUInt32-At($offset) {
        $ms.Position = $offset
        return $reader.ReadUInt32()
    }

    # --- Validate or patch geometry fields ---
    $sectorsPerTrack = ReadUInt16-At 0x18
    if($sectorsPerTrack -lt 1 -or $sectorsPerTrack -gt 255){Write-At 0x18 {$writer.Write([uint16]63)}}

    $numHeads = ReadUInt16-At 0x1A
    if($numHeads -lt 1 -or $numHeads -gt 255){Write-At 0x1A {$writer.Write([uint16]255)}}

    $hiddenSectors = ReadUInt32-At 0x1C
    if($hiddenSectors -eq 0xFFFFFFFF){Write-At 0x1C {$writer.Write([uint32]0)}}

    # --- Always update core FAT32 BPB fields ---
    Write-At 0x03 {$writer.Write([System.Text.Encoding]::ASCII.GetBytes("BLOCKYOS"))}
    Write-At 0x0B {$writer.Write([uint16]$sectorSize)}
    Write-At 0x0D {$writer.Write([byte]$sectorsPerCluster)}
    Write-At 0x0E {$writer.Write([uint16]$reservedSectors)}
    Write-At 0x10 {$writer.Write([byte]$fatCopies)}
    Write-At 0x20 {$writer.Write([uint32]$totalSectors)}
    Write-At 0x24 {$writer.Write([uint32]$fatSizeInSectors)}
    Write-At 0x2C {$writer.Write([uint32]2)} # root cluster
    Write-At 0x30 {$writer.Write([uint16]1)} # FSInfo
    Write-At 0x32 {$writer.Write([uint16]6)} # backup boot sector

    # Volume ID
    Write-At 0x43 {$writer.Write([uint32](Get-Random))}

    # Volume label
    $labelBytes = [System.Text.Encoding]::ASCII.GetBytes($VolumeLabel.PadRight(11).Substring(0,11))
    Write-At 0x47 {$writer.Write($labelBytes)}

    # FS type
    Write-At 0x52 {$writer.Write([System.Text.Encoding]::ASCII.GetBytes("FAT32   "))}

    # Boot signature
    Write-At 510 {
        $writer.Write([byte]0x55)
        $writer.Write([byte]0xAA)
    }
    return $bootbinaries
}

function Get-FAT32ClusterSize{
	param([uint64]$PartitionSizeBytes)

	$sectorSize = 512
	$candidateSizes = @(512, 1024, 2048, 4096)

	foreach($clusterSize in $candidateSizes){
		$sectorsPerCluster = [byte]($clusterSize / $sectorSize)
		$totalSectors = [uint32]($PartitionSizeBytes / $sectorSize)
		$fatSizeInSectors = 0
		$previousFatSize = -1

		do{
			$previousFatSize = $fatSizeInSectors
			$dataSectors = $totalSectors - 32 - (2 * $fatSizeInSectors)
			if($dataSectors -lt 0){break}

			$clusterCount = [uint32][Math]::Floor($dataSectors / $sectorsPerCluster)
			$fatSizeInSectors = [uint32][Math]::Ceiling((($clusterCount + 2) * 4.0) / $sectorSize)
		}while($fatSizeInSectors -ne $previousFatSize)

		if($clusterCount -ge 65525){return $clusterSize}
	}

	throw "Partition too small for FAT32. Increase the partition size or use FAT16/NTFS."
}

function Create-FAT16BootSector {
    param(
        [uint64]$PartitionSizeBytes,
        [string]$VolumeLabel = "BLOCKYOS",
        [uint32]$ClusterSize = 4096,
        [byte[]]$BootCode
    )

    $sectorSize = 512
    $sectorsPerCluster = [byte]($ClusterSize / $sectorSize)
    $reservedSectors = 1
    $fatCopies = 2
    $rootEntries = 512
    $totalSectors = [uint32]($PartitionSizeBytes / $sectorSize)

    # FAT16 FAT size estimate
    $fatSizeInSectors = [uint16][Math]::Ceiling(($totalSectors * 2.0) / ($sectorSize * 8.0))

    # Load boot code or blank
    $bootbinaries = $null
    if($BootCode){$bootbinaries = $BootCode
    }else{$bootbinaries = New-Object byte[] 512}
    $ms = New-Object System.IO.MemoryStream($bootbinaries, 0, $bootbinaries.Length, $true)
    $writer = New-Object System.IO.BinaryWriter($ms)
    $reader = New-Object System.IO.BinaryReader($ms)

    # Helpers
    function Write-At($offset, [ScriptBlock]$action){
        $ms.Position = $offset
        & $action
    }
    function ReadUInt16-At($offset){
        $ms.Position = $offset
        return $reader.ReadUInt16()
    }
    function ReadUInt32-At($offset){
        $ms.Position = $offset
        return $reader.ReadUInt32()
    }

    # -------------------------------
    # Validate or patch geometry fields
    # -------------------------------

    # Sectors per track (offset 0x18)
    $spt = ReadUInt16-At 0x18
    if($spt -lt 1 -or $spt -gt 255){
        Write-At 0x18 {$writer.Write([uint16]63)}
    }

    # Number of heads (offset 0x1A)
    $heads = ReadUInt16-At 0x1A
    if($heads -lt 1 -or $heads -gt 255){
        Write-At 0x1A {$writer.Write([uint16]255)}
    }

    # Hidden sectors (offset 0x1C)
    $hidden = ReadUInt32-At 0x1C
    if ($hidden -eq 0xFFFFFFFF) {
        Write-At 0x1C {$writer.Write([uint32]0)}
    }

    # -------------------------------
    # Always update FAT16‑critical BPB fields
    # -------------------------------
    # OEM name
    Write-At 0x03 {$writer.Write([System.Text.Encoding]::ASCII.GetBytes("BLOCKYOS")) }

    # Bytes per sector
    Write-At 0x0B {$writer.Write([uint16]$sectorSize) }

    # Sectors per cluster
    Write-At 0x0D {$writer.Write([byte]$sectorsPerCluster) }

    # Reserved sectors
    Write-At 0x0E {$writer.Write([uint16]$reservedSectors) }

    # FAT copies
    Write-At 0x10 {$writer.Write([byte]$fatCopies) }

    # Root entries
    Write-At 0x11 {$writer.Write([uint16]$rootEntries) }

    # Total sectors (16‑bit)
    Write-At 0x13 {
        if($totalSectors -le 0xFFFF){
            $writer.Write([uint16]$totalSectors)
        }else{$writer.Write([uint16]0)}
    }

    # Media descriptor
    Write-At 0x15 {$writer.Write([byte]0xF8)}

    # FAT size
    Write-At 0x16 {$writer.Write([uint16]$fatSizeInSectors)}

    # Total sectors (32‑bit)
    Write-At 0x20 {
        if($totalSectors -gt 0xFFFF){
            $writer.Write([uint32]$totalSectors)
        }else{$writer.Write([uint32]0)}
    }

    # Drive number + signature
    Write-At 0x24 {
        $writer.Write([byte]0x80)
        $writer.Write([byte]0)
        $writer.Write([byte]0x29)
    }

    # Volume ID
    Write-At 0x27 {$writer.Write([uint32](Get-Random))}

    # Volume label
    $labelBytes = [System.Text.Encoding]::ASCII.GetBytes($VolumeLabel.PadRight(11).Substring(0,11))
    Write-At 0x2B {$writer.Write($labelBytes)}

    # FS type
    Write-At 0x36 {$writer.Write([System.Text.Encoding]::ASCII.GetBytes("FAT16   "))}

    # Boot signature
    Write-At 510 {
        $writer.Write([byte]0x55)
        $writer.Write([byte]0xAA)
    }
    return $bootbinaries
}

function Create-NTFSBootSector {
	param(
		[uint64]$PartitionSizeBytes,
		[string]$VolumeLabel = "BLOCKYOS"
	)

	$sectorSize = 512
	$clusterSize = 4096
	$sectorsPerCluster = [byte]($clusterSize / $sectorSize)
	$totalSectors = [uint64]($PartitionSizeBytes / $sectorSize)
	$mftStartCluster  = [uint64]800
	$mft2StartCluster = [uint64]801
	$clustersPerMFTRecord = [sbyte]-10

	$bootbinaries = New-Object byte[] 512
	$ms = New-Object System.IO.MemoryStream($bootbinaries, 0, 512, $true)
	$writer = New-Object System.IO.BinaryWriter($ms)

	$writer.Write([byte]0xEB)
	$writer.Write([byte]0x52)
	$writer.Write([byte]0x90)

	$oemBytes = [System.Text.Encoding]::ASCII.GetBytes("NTFS    ")
	$writer.Write($oemBytes,0,8)

	$writer.Write([uint16]$sectorSize)
	$writer.Write([byte]$sectorsPerCluster)
	$writer.Write([uint16]0)

	$writer.Write([byte]0)
	$writer.Write([uint16]0)
	$writer.Write([uint16]0)
	$writer.Write([uint16]0)

	$writer.Write([byte]0xF8)
	$writer.Write([uint16]0)
	$writer.Write([uint16]0)
	$writer.Write([uint16]0)

	$writer.Write([uint32]0)
	$writer.Write([uint32]0)

	$writer.Write([uint64]$totalSectors)
	$writer.Write([uint64]$mftStartCluster)
	$writer.Write([uint64]$mft2StartCluster)

	$writer.Write([sbyte]$clustersPerMFTRecord)
	$writer.Write((New-Object byte[] 3),0,3)

	$writer.Write([byte]8)
	$writer.Write((New-Object byte[] 3),0,3)

	$writer.Write([uint64](Get-Random))
	$writer.Write([uint32]0)

	if($BootCode){
		$code = [System.IO.File]::ReadAllBytes($BootCode)
		$remaining = 512 - ($writer.BaseStream.Position + 2)
		$writer.Write($code, 0, [Math]::Min($code.Length, $remaining))
		if ($code.Length -lt $remaining) {
			$writer.Write((New-Object byte[] ($remaining - $code.Length)), 0, $remaining - $code.Length)
		}
	}
	else{
		$writer.Write((New-Object byte[] (512 - ($writer.BaseStream.Position + 2))), 0, 512 - ($writer.BaseStream.Position + 2))
	}

	$writer.Write([byte]0x55)
	$writer.Write([byte]0xAA)

	$writer.Flush()
	return $bootbinaries
}

function Write-FATDirectoryRecursive{
	param(
		[System.IO.FileStream]$Disk,
		[string]$HostPath,
		[uint32]$CurrentCluster,
		[uint32]$ClusterSize,
		[uint64]$DataStartOffset,
		[uint32[]]$FatTable,
		[uint32]$ParentCluster = 0
	)

	function Write-Cluster{
		param([uint32]$Cluster,[byte[]]$Data)
		$offset = $DataStartOffset + (($Cluster - 2) * $ClusterSize)
		$Disk.Seek([int64]$offset,'Begin') | Out-Null
		$Disk.Write($Data,0,$Data.Length)
	}

	function Allocate-Cluster{
		param([uint32[]]$Fat)
		for($i=2; $i -lt $Fat.Length; $i++){
			if($Fat[$i] -eq 0){
				$Fat[$i] = 0x0FFFFFFF
				return [uint32]$i
			}
		}
		throw "Out of clusters"
	}

	function Append-Cluster{
		param([uint32]$ChainStart,[uint32[]]$Fat)
		$c = $ChainStart
		while($Fat[$c] -ne 0x0FFFFFFF){$c = $Fat[$c]}
		$new = Allocate-Cluster -Fat $Fat
		$Fat[$c] = $new
		return $new
	}

	function New-DirEntry {
		param(
			[string]$Name,
			[switch]$IsDirectory,
			[uint32]$StartCluster,
			[uint32]$Size = 0
		)

		$e = New-Object byte[] 32

		if ($Name -eq ".") {
			[Text.Encoding]::ASCII.GetBytes(".          ").CopyTo($e, 0)
		} elseif ($Name -eq "..") {
			[Text.Encoding]::ASCII.GetBytes("..         ").CopyTo($e, 0)
		} else {
			$base = $Name
			$ext  = ""

			if ($Name.Contains(".")) {
				$parts = $Name.Split(".", 2)
				$base = $parts[0]
				$ext  = $parts[1]
			}

			$base = ($base.ToUpper() -replace '[^A-Z0-9]', '_')
			$ext  = ($ext.ToUpper()  -replace '[^A-Z0-9]', '_')

			if ($base.Length -gt 8) { $base = $base.Substring(0,8) }
			if ($ext.Length  -gt 3) { $ext  = $ext.Substring(0,3) }

			$base = $base.PadRight(8)
			$ext  = $ext.PadRight(3)

			[Text.Encoding]::ASCII.GetBytes($base).CopyTo($e,0)
			[Text.Encoding]::ASCII.GetBytes($ext).CopyTo($e,8)
		}

		$e[11] = if ($IsDirectory) { 0x10 } else { 0x20 }

		$c = [uint32]$StartCluster
		[BitConverter]::GetBytes([uint16]($c -shr 16)).CopyTo($e,20)
		[BitConverter]::GetBytes([uint16]($c -band 0xFFFF)).CopyTo($e,26)

		[BitConverter]::GetBytes($Size).CopyTo($e,28)

		return $e
	}

	$items   = Get-ChildItem -LiteralPath $HostPath -Force
	$entries = New-Object System.Collections.Generic.List[byte[]]

	$parentForDotDot = if ($ParentCluster -ne 0) { $ParentCluster } else { $CurrentCluster }

	$entries.Add( (New-DirEntry -Name "."  -IsDirectory -StartCluster $CurrentCluster -Size 0) )
	$entries.Add( (New-DirEntry -Name ".." -IsDirectory -StartCluster $parentForDotDot -Size 0) )

	foreach ($item in $items) {
		if($item.Name -eq "." -or $item.Name -eq ".."){continue}

		if($item.PSIsContainer){
			$newCluster = Allocate-Cluster -Fat $FatTable
			$entries.Add( (New-DirEntry -Name $item.Name -IsDirectory -StartCluster $newCluster -Size 0) )

			Write-FATDirectoryRecursive `
				-Disk $Disk `
				-HostPath $item.FullName `
				-CurrentCluster $newCluster `
				-ClusterSize $ClusterSize `
				-DataStartOffset $DataStartOffset `
				-FatTable $FatTable `
				-ParentCluster $CurrentCluster
		} else {
			$bytes     = [System.IO.File]::ReadAllBytes($item.FullName)
			$remaining = $bytes.Length
			$firstCluster = Allocate-Cluster -Fat $FatTable
			$cluster = $firstCluster
			$pos = 0

			while($remaining -gt 0){
				$chunk = [Math]::Min($ClusterSize,$remaining)
				$buf   = New-Object byte[] $ClusterSize
				[Array]::Copy($bytes,$pos,$buf,0,$chunk)
				Write-Cluster -Cluster $cluster -Data $buf
				$remaining -= $chunk
				$pos       += $chunk
				if($remaining -gt 0){
					$cluster = Append-Cluster -ChainStart $cluster -Fat $FatTable
				}
			}

			$entries.Add( (New-DirEntry -Name $item.Name -StartCluster $firstCluster -Size $bytes.Length) )
		}
	}

	$dirBytes = New-Object System.IO.MemoryStream
	foreach ($e in $entries) { $dirBytes.Write($e,0,$e.Length) }
	$dirBytes.Flush()
	$data = $dirBytes.ToArray()

	$neededClusters = [uint32][Math]::Ceiling($data.Length / $ClusterSize)
	$cluster = $CurrentCluster
	$pos = 0

	for ($i=0; $i -lt $neededClusters; $i++) {
		$chunk = [Math]::Min($ClusterSize, $data.Length - $pos)
		$buf   = New-Object byte[] $ClusterSize
		[Array]::Copy($data,$pos,$buf,0,$chunk)
		Write-Cluster -Cluster $cluster -Data $buf
		$pos += $chunk
		if ($i -lt $neededClusters-1) {
			$cluster = Append-Cluster -ChainStart $cluster -Fat $FatTable
		}
	}
}

switch($FileSystemType){
	"FAT16" {
		Log-Message "Formatting as FAT16"
		$boot = Create-FAT16BootSector -PartitionSizeBytes $partitionSizeBytes -VolumeLabel $VolumeName
		Log-Message -Message "Writing Boot Sector at: $([Int64]$partitionOffsetBytes)"
		$fs.Seek([int64]$partitionOffsetBytes,'Begin') | Out-Null
		$fs.Write($boot,0,$boot.Length)

		# FAT16 doesn't strictly use FSINFO, but we can skip it to stay spec-clean
		$clusterSize = 4096
		$clusters = [uint32]($partitionSizeBytes / $clusterSize)
		$fatSizeBytes = $clusters * 2
		$fatSectors = [uint32][Math]::Ceiling($fatSizeBytes / $sectorSize)
		$fatTable = New-Object uint32[] $clusters

		if ($SourceDirectory) {
			$rootDirSectors = (512 * 32 / $sectorSize)
			$dataOffset = $partitionOffsetBytes + (1 + (2 * $fatSectors) + $rootDirSectors) * $sectorSize
			Write-FATDirectoryRecursive -Disk $fs.BaseStream -HostPath $SourceDirectory -CurrentCluster 2 -ClusterSize $clusterSize -DataStartOffset $dataOffset -FatTable $fatTable -ParentCluster 2
		}

		# TODO: write FAT + root dir for FAT16 if you want Windows to fully trust it
	}
	"FAT32" {
		Log-Message "Formatting as FAT32"
		$clusterSize = Get-FAT32ClusterSize -PartitionSizeBytes $partitionSizeBytes
		$boot = Create-FAT32BootSector -BootCode $BootCode -PartitionSizeBytes $partitionSizeBytes -VolumeLabel $VolumeName -ClusterSize $clusterSize
		Log-Message -Message "Writing Boot Sector at: $([Int64]$partitionOffsetBytes)"
		$fs.Seek([int64]$partitionOffsetBytes,'Begin') | Out-Null
		$fs.Write($boot,0,$boot.Length)

		$backupBootOffset = $partitionOffsetBytes + (6 * 512)
		Log-Message -Message "Writing Backup Boot Sector at: $([Int64]$backupBootOffset)"
		$fs.Seek([int64]$backupBootOffset,'Begin') | Out-Null
		$fs.Write($boot,0,512)

		Log-Message "Writing FSINFO Sector..."
		$fsinfo = New-Object byte[] 512

		[BitConverter]::GetBytes([uint32]0x41615252).CopyTo($fsinfo, 0)
		[BitConverter]::GetBytes([uint32]0x61417272).CopyTo($fsinfo, 484)
		[BitConverter]::GetBytes([uint32]0xFFFFFFFFu).CopyTo($fsinfo, 488)
		[BitConverter]::GetBytes([uint32]0x00000003).CopyTo($fsinfo, 492)
		[BitConverter]::GetBytes([uint32]0xAA550000u).CopyTo($fsinfo, 508)

		$fs.Seek([int64]($partitionOffsetBytes + 512), 'Begin') | Out-Null
		$fs.Write($fsinfo, 0, 512)

		$sectorsPerCluster = [byte]($clusterSize / $sectorSize)
		$fatSizeInSectors = 0
		$previousFatSize = -1
		$maxIterations = 100
		$iteration = 0
		do{
			$previousFatSize = $fatSizeInSectors
			$dataSectors = $partitionSizeSectors - 32 - (2 * $fatSizeInSectors)
			$clusters = [uint32][Math]::Floor($dataSectors / $sectorsPerCluster)
			$fatSizeInSectors = [uint32][Math]::Ceiling((($clusters + 2) * 4.0) / $sectorSize)
			$iteration++
			if($iteration -gt $maxIterations){throw "FAT32 FAT table sizing did not converge after $maxIterations iterations"}
		}while($fatSizeInSectors -ne $previousFatSize)

		$fatSectors = $fatSizeInSectors
		$fatTable = New-Object uint32[] $clusters

		if($clusters -gt 0){$fatTable[0] = 0x0FFFFFF8}
		if($clusters -gt 1){$fatTable[1] = (-not 0)}
		if($clusters -gt 2){$fatTable[2] = 0x0FFFFFFF}

		$reservedSectors = 32
		$fatStartOffset = $partitionOffsetBytes + ($reservedSectors * $sectorSize)
		$dataStartOffset = $fatStartOffset + (2 * $fatSectors * $sectorSize)

		if($SourceDirectory){
			Write-FATDirectoryRecursive -Disk $fs.BaseStream -HostPath $SourceDirectory -CurrentCluster 2 -ClusterSize $clusterSize -DataStartOffset $dataStartOffset -FatTable $fatTable -ParentCluster 2
		}

		$fatBytes = New-Object byte[] ($fatSectors * $sectorSize)
		for ($i=0; $i -lt $fatTable.Length; $i++) {
			[BitConverter]::GetBytes($fatTable[$i]).CopyTo($fatBytes,$i*4)
		}
		for ($copy=0; $copy -lt 2; $copy++) {
			$off = $fatStartOffset + ($copy * $fatSectors * $sectorSize)
			$fs.Seek([int64]$off,'Begin') | Out-Null
			$fs.Write($fatBytes,0,$fatBytes.Length)
		}
	}
	"NTFS" {
		Log-Message "Formatting as NTFS (boot sector only; full NTFS loader not implemented)"
		$boot = Create-NTFSBootSector -PartitionSizeBytes $partitionSizeBytes -VolumeLabel $VolumeName
		Log-Message -Message "Writing Boot Sector at: $([Int64]$partitionOffsetBytes)"
		$fs.Seek([int64]$partitionOffsetBytes,'Begin') | Out-Null
		$fs.Write($boot,0,$boot.Length)
		if ($SourceDirectory) {
			Log-Message "NTFS recursive loader not implemented – requires full MFT implementation" "WARNING"
		}
	}
}

$br.Close()
$fs.BaseStream.Close()

Log-Message "Partition FS Driver Completed Successfully"
Write-Host "Partition formatted: $DiskImage (Partition '$PartitionName', Flag '$PartitionFlag', FS $FileSystemType)"
if ($Verbose -and $LogFile) {
	Write-Host "Operations logged to: $LogFile"
}
