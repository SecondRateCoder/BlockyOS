param(
	[string]$LayoutJson,
	[string]$OutputImage,
	[string]$LogFile,
	[switch]$Verbose,
	[switch]$IsDrive,
	[switch]$Help
)

function Pad-Array{
	param(
		[array]$Array,
		[int]$Size,
		$PadValue = $null
	)

	if($Array.Count -lt $Size){$Array += ,$PadValue * ($Size - $Array.Count)}
	return $Array
}

function Compute-CRC32{
	param([byte[]]$Data)

	$table = @(0..255 | ForEach-Object{
		$crc = $_
		for($i=0; $i -lt 8; $i++){
			if($crc -band 1){$crc = (0xEDB88320 -bxor ($crc -shr 1))
			}else{$crc = ($crc -shr 1)}
		}
		$crc
	})

	$crc32 = 0xFFFFFFFF
	foreach($b in $Data){
		$crc32 = ($crc32 -shr 8) -bxor $table[($crc32 -bxor $b) -band 0xFF]
	}

	return (-bnot $crc32) -band 0xFFFFFFFF
}

# ============================================================
# HELP
# ============================================================
function Show-Help{
@"
GPT/MBR Disk Builder

SYNOPSIS
	Creates a GPT disk image (with protective MBR) or formats a real drive
	using a JSON partition layout.

USAGE
	# Build disk image
	.\GPT.ps1 -LayoutJson layout.json -OutputImage disk.img

	# Format physical drive (DANGEROUS)
	.\GPT.ps1 -LayoutJson layout.json -OutputImage \\.\PhysicalDrive2 -IsDrive

JSON FORMAT
{
  "partitions": [
	{
	  "name": "EFI",
	  "size": 33554432,							In 512-byte Blocks
	  "type": "<GUID>",
	  "attributes": ["efi-boot", "system"],
	  "give": ["fit", "grow"]
	}
  ]
}

SUPPORTED PARTITION ATTRIBUTES
	system           : Required for platform to function (bit 0)
	required         : Required for OS (bit 0)
	firmware-ignore  : Firmware should ignore this partition (bit 1)
	legacy-boot      : Legacy BIOS bootable (bit 2)
	efi-boot         : EFI/boot partition (treated as system)
	readonly         : Read-only partition (bit 48)
	hidden           : Hidden partition (bit 49)
	0xHEXVALUE       : Custom hex attribute value

SUPPORTED "give" DIRECTIVES
	fit     : If partition causes overflow, shrink to fit remaining available space
	grow    : Expand partition to fill all remaining available space (only one partition)
	fit+grow: First fit if needed, then grow to fill remaining space

PARAMETERS
	-LayoutJson   		   	Path to JSON layout
	-OutputImage           	Path to disk image or \\.\PhysicalDriveX
	-IsDrive               	Write directly to a real drive (DANGEROUS)
	-LogFile               	Log output
	-Verbose               	Verbose logging
	-Help                  	Show this help
	-ProtectiveMBR         	File Path to a Protective MBR Binary

"@
}

function LBA-2-CHS{
	param(
		[uint64]$LBA,
		[byte]$HeadsPerCylinder = 255,
		[byte]$SectorsPerTrack = 63
	)
	return [pscustomobject]@{
		cylinder    = [math]::Min([math]::Floor($LBA / ($HeadsPerCylinder * $SectorsPerTrack)), 255)
		head        = [math]::Min([math]::Floor(($LBA / $SectorsPerTrack) % $HeadsPerCylinder), 255)
		sector      = [math]::Min([math]::Floor(($LBA % $SectorsPerTrack) + 1), 255)
	}
}

function New-ProtectiveMBRFromJson{
    param(
        [Parameter(Mandatory=$true)]
        [string]$JsonPath,

        # Optional: total disk size in bytes (if not provided, computed from JSON)
        [UInt64]$DiskSizeBytes
    )

    if(-not (Test-Path $JsonPath)){throw "JSON file not found: $JsonPath"}

    $layout = Get-Content $JsonPath -Raw | ConvertFrom-Json -AsHashtable
    $parts  = $layout.partitions

    if(-not $parts -or $parts.Count -eq 0){throw "JSON contains no partitions"}

    # Compute total disk size if not provided
    if(-not $DiskSizeBytes){$DiskSizeBytes = ($parts | Measure-Object -Property size -Sum).Sum}

    $sectorSize = 512
    $totalSectors = [uint32]([math]::Floor($DiskSizeBytes / $sectorSize))

    # Create empty 512-byte MBR
    $mbr = New-Object byte[] 512

    # Boot signature
    $mbr[510] = [UInt16]0x55
    $mbr[511] = [UInt16]0xAA

    # Partition entry offset
    $entryOffset = 446

    # Helper: write a 16-byte MBR entry
    function Write-MbrEntry{
        param(
            [byte[]]$Buffer,
            [int]$Offset,
            [byte]$Status,
            [byte]$Type,
            [uint32]$StartLBA,
            [uint32]$SectorCount
        )

        # Status
        $Buffer[$Offset + 0] = $Status

        $Buffer[$Offset + 1] = [byte]($StartLBA / (255 * 63))
        $Buffer[$Offset + 2] = [byte](($StartLBA / 255) % 63)
        $Buffer[$Offset + 3] = [byte](($StartLBA % 255) + 1)

        # Type
        $Buffer[$Offset + 4] = [byte]$Type
        $CHS = LBA-2-CHS $StartLBA
        $Buffer[$Offset + 5] = [byte]$CHS.cylinder
        $Buffer[$Offset + 6] = [byte]$CHS.head
        $Buffer[$Offset + 7] = [byte]$CHS.sector

        # LBA start
        [BitConverter]::GetBytes($StartLBA).CopyTo($Buffer, $Offset + 8)

        # Sector count
        [BitConverter]::GetBytes($SectorCount).CopyTo($Buffer, $Offset + 12)
    }

    # Compute LBA positions for each partition
    $currentLBA = 2048  # Standard alignment
    $computed = @()

    foreach($p in $parts){
        $sectors = [uint32]([math]::Ceiling($p.size / $sectorSize))
        $computed += [PSCustomObject]@{
            name   = $p.name
            size   = $p.size
            start  = $currentLBA
            end    = $currentLBA + $sectors - 1
            sectors= $sectors
            type   = $p.type
        }
        $currentLBA += $sectors
    }

    # If more than 4 partitions → last entry spans all mapped space
    if($computed.Count -gt 4){
        $first = $computed[0]
        $last  = $computed[-1]

        $computed = $computed[0..2] + @(
            [PSCustomObject]@{
                name    = "MBR-SPAN"
                size    = $DiskSizeBytes
                start   = $first.start
                end     = $last.end
                sectors = [uint32]($last.end - $first.start + 1)
            }
        )
    }

    # Write up to 4 entries
    for($i = 0; $i -lt [math]::Min(4, $computed.Count); $i++){
        # $p = $computed[$i]
        $type = if($computed[$i].type -contains 'efi-boot'){0x44}else{0x00}
        Write-MbrEntry -Buffer $mbr -Offset ($entryOffset + (16 * $i)) -Status 0x00 -Type $type -StartLBA ([uint32]($computed[$i]).start) -SectorCount ([uint32]($computed[$i]).sectors)
    }

    # Boot signature
    $mbr[510] = [UInt16]0x55
    $mbr[511] = [UInt16]0xAA

    return $mbr
}

if ($Help){Show-Help; exit}

# ============================================================
# LOGGING
# ============================================================
$logBuffer = @()

function Log{
    param([string]$Msg, [string]$Level = "INFO", [System.ConsoleColor]$color)

    $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $line = "[$ts] [$Level] $Msg"

    if($Verbose){if($color){Write-Host $line -ForegroundColor $color}else{Write-Host $line}}
    if($LogFile){Add-Content -Path $LogFile -Value $line}

    $logBuffer += $line
}

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
        Log -Info "WARNING" -color Yellow ">> [SEEK] Offset=$offset Origin=$origin  (Caller: $((Get-PSCallStack)[1].Location))"

        return $this.BaseStream.Seek($offset, $origin)
    }

    $proxy | Add-Member -MemberType ScriptMethod -Name Write -Value {
        param([byte[]]$buffer, $offset, $count)

        if(($count -eq 0) -or (-not $count)){$count = $buffer.Count - $offset}
        $caller = (Get-PSCallStack)[1].Location
        $abs = $this.BaseStream.Position

        Log -Info "WARNING" -color Yellow ">> ([WRITE] Position=$abs  Count=$count  Caller=$caller)"

        return $this.BaseStream.Write($buffer, $offset, $count)
    }

    

    return $proxy
}

if($LogFile){"=== GPT/MBR Driver Session ===" | Out-File -FilePath $LogFile -Force}

Log "Driver started"

# ============================================================
# BASIC PARAM VALIDATION
# ============================================================
if(-not $LayoutJson){
    Log "LayoutJson is required" "ERROR"
    exit 1
}

if(-not $OutputImage){
    Log "OutputImage is required" "ERROR"
    exit 1
}

if(-not (Test-Path $LayoutJson)){
    Log "Partition layout JSON not found: $LayoutJson" "ERROR"
    exit 1
}

# ============================================================
# LOAD JSON
# ============================================================
try{$layout = Get-Content $LayoutJson -Raw | ConvertFrom-Json -AsHashtable
}catch{
    Log "Failed to parse JSON: $_" "ERROR"
    exit 1
}

$parts = $layout.partitions
if(-not $parts -or $parts.Count -eq 0){
    Log "JSON contains no partitions" "ERROR"
    exit 1
}

Log "Loaded $($parts.Count) partition definitions"

# ============================================================
# ATTRIBUTE MAPPING
# ============================================================
function Get-GptAttributeValue{
    param([object]$Attributes)

    if(-not $Attributes){return [uint64]0}

    $attrs = @()
    if($Attributes -is [string]){$attrs = @($Attributes)
    }else{$attrs = @($Attributes)}

    [uint64]$value = 0

    foreach($a in $attrs){
        switch($a){
            "system"         {$value = $value -bor 0x0000000000000001UL}
            "required"       {$value = $value -bor 0x0000000000000001UL}
            "efi-boot"       {$value = $value -bor 0x0000000000000001UL}
            "firmware-ignore"{$value = $value -bor 0x0000000000000002UL}
            "legacy-boot"    {$value = $value -bor 0x0000000000000004UL}
            "readonly"       {$value = $value -bor 0x0001000000000000UL}
            "hidden"         {$value = $value -bor 0x0002000000000000UL}
            default{
                if($a -match "^0x[0-9A-Fa-f]+$"){
                    $value = $value -bor ([UInt64]::Parse($a.Substring(2), "HexNumber"))
                }
            }
        }
    }

    return $value
}

# ============================================================
# NORMALIZE "give" DIRECTIVES
# ============================================================
function Normalize-Give{
    param([object]$Give)

    if(-not $Give){return @()}
    if($Give -is [string]){return @($Give)}
    return @($Give)
}

# ============================================================
# DISK LAYOUT COMPUTATION (INITIAL)
# ============================================================
$diskGuid = [Guid]$layout.disk.type

$sectorSize     = 512
$firstUsableLBA = 2048
$currentLBA     = $firstUsableLBA

foreach($p in $parts){
    $p.give            = Normalize-Give $p.give
    $p.attributesValue = Get-GptAttributeValue $p.attributes

    [uint64]$sectors = [math]::Ceiling($p.size / $sectorSize)
    $p.sectors  = $sectors
    $p.startLBA = [uint64]$currentLBA
    $p.endLBA   = [uint64]($currentLBA + $sectors - 1)

    $currentLBA = $p.endLBA + 1
}

$lastEndInitial = ($parts | Sort-Object endLBA -Descending)[0].endLBA

# GPT partition entry layout
$entryCount       = $parts.Count
$entrySize        = 128
$entriesPerSector = $sectorSize / $entrySize
$entrySectors     = [math]::Ceiling($entryCount / $entriesPerSector)    

# Base disk sectors (before "grow"): partitions + backup GPT area
$diskSectorsBase = [uint64]($lastEndInitial + 1 + $entrySectors + 1)

# ============================================================
# APPLY "FIT" AND "GROW"
# ============================================================
function Apply-GiveDirectives{
    param(
        [array]$Parts,
        [uint64]$DiskSectors,
        [uint64]$FirstUsableLBA,
        [int]$SectorSize
    )

    $usableSectors = $DiskSectors - $FirstUsableLBA

    # Total requested sectors
    [uint64]$totalRequested = ($Parts | Measure-Object -Property sectors -Sum).Sum

    # FIT: shrink fit-partitions if total exceeds usable
    if($totalRequested -gt $usableSectors){
        [uint64]$overflow = $totalRequested - $usableSectors

        foreach($p in ($Parts | Where-Object { $_.give -contains "fit" })){
            if($overflow -le 0){break}

            [uint64]$canReduce = if ($p.sectors -gt 1) { $p.sectors - 1 } else { 0 }
            if ($canReduce -le 0) { continue }

            [uint64]$reduceBy = [math]::Min($canReduce, $overflow)
            $p.sectors -= $reduceBy
            $overflow  -= $reduceBy
        }

        if($overflow -gt 0){
            Log "Unable to fit partitions within disk using 'fit' directives" "ERROR"
            exit 1
        }
    }

    # GROW: only one partition allowed
    $growParts = $Parts | Where-Object{$_.give -contains "grow"}
    if($growParts.Count -gt 1){
        Log "Multiple 'grow' partitions detected — only one allowed" "ERROR"
        exit 1
    }

    [uint64]$totalAfterFit = ($Parts | Measure-Object -Property sectors -Sum).Sum
    [uint64]$remaining     = $usableSectors - $totalAfterFit

    if($growParts.Count -eq 1 -and $remaining -gt 0){
        $gp = $growParts[0]
        $gp.sectors += $remaining
    }

    # Recompute LBAs sequentially
    [uint64]$cur = $FirstUsableLBA
    foreach($p in $Parts){
        $p.startLBA = $cur
        $p.endLBA   = $cur + $p.sectors - 1
        $cur        = $p.endLBA + 1
    }
}

Apply-GiveDirectives -Parts $parts -DiskSectors $diskSectorsBase -FirstUsableLBA $firstUsableLBA -SectorSize $sectorSize

$lastUsableLBA = ($parts | Sort-Object endLBA -Descending)[0].endLBA

# Backup GPT placement after last usable LBA
# $backupEntriesLBA = $lastUsableLBA + 1
# $backupHeaderLBA  = $backupEntriesLBA + $entrySectors
# $totalSectors     = $backupHeaderLBA + 1
$entriesBytes   = $entryCount * $entrySize
$entrySectors   = [math]::Ceiling($entriesBytes / $sectorSize)
# Final LBA of disk
$backupHeaderLBA = $lastUsableLBA + 1 + $entrySectors
$totalSectors    = $backupHeaderLBA + 1
# Backup entries go immediately before backup header
$backupEntriesLBA = $backupHeaderLBA - $entrySectors

Log "Computed disk layout:"
Log "  First usable LBA : $firstUsableLBA"
Log "  Last usable LBA  : $lastUsableLBA"
Log "  Total sectors    : $totalSectors"
Log "  Entry sectors    : $entrySectors"

# ============================================================
# OPEN OUTPUT TARGET
# ============================================================
$access = [System.IO.FileAccess]::ReadWrite

if($IsDrive){
    Log "Opening physical drive: $OutputImage"
    try{$fs = New-Object System.IO.FileStream($OutputImage, [System.IO.FileMode]::Open, $access, [System.IO.FileShare]::ReadWrite)
    }catch{
        Log "Failed to open physical drive: $_" "ERROR"
        exit 1
    }
}else{
    Log "Creating disk image: $OutputImage"
    try{
        $fs = New-LoggedFileStream $OutputImage
        $fs.BaseStream.SetLength([int64]($totalSectors * $sectorSize))
    }catch{
        Log "Failed to create disk image: $_" "ERROR"
        exit 1
    }
}


function Seek-Bytes{
    param([UInt64]$Offset)
    $fs.Seek([int64]$Offset, [System.IO.SeekOrigin]::Begin) | Out-Null
}

# ============================================================
# WRITE PROTECTIVE MBR
# ============================================================
Log "Writing protective MBR"

# Create a clean 512-byte buffer
$ProtectiveMBR = $ProtectiveMBR = New-ProtectiveMBRFromJson -JsonPath $LayoutJson
$ProtectiveMBR[510] = 0x55
$ProtectiveMBR[511] = 0xAA

# Define the single Protective Partition (Entry 0)
$mbrOffset = 446
$ProtectiveMBR[$mbrOffset + 4] = 0xEE # Type: GPT Protective
$CHS = LBA-2-CHS 1
$ProtectiveMBR[$mbrOffset + 1] = $CHS.cylinder
$ProtectiveMBR[$mbrOffset + 2] = $CHS.head
$ProtectiveMBR[$mbrOffset + 3] = $CHS.sector

# Start LBA must be 1
[BitConverter]::GetBytes([uint32]1).CopyTo($ProtectiveMBR, $mbrOffset + 8)

# Size should cover the whole disk (totalSectors - 1)
$mbrSize = if ($totalSectors -gt 0xFFFFFFFF) { 0xFFFFFFFF } else { [uint32]($totalSectors - 1) }
[BitConverter]::GetBytes($mbrSize).CopyTo($ProtectiveMBR, $mbrOffset + 12)

Seek-Bytes 0
$fs.Write($ProtectiveMBR)

# ============================================================
# GPT HEADER + ENTRIES HELPERS
# ============================================================
function Write-GptHeader {
    param(
        [UInt64]$HeaderLBA,
        [UInt64]$BackupLBA,
        [UInt64]$EntriesLBA,
        [UInt64]$FirstUsableLBA,
        [UInt64]$LastUsableLBA,
        [UInt32]$EntryCount,
        [UInt32]$EntrySize,
        [Guid]$DiskGuid,
        [Int32]$EntriesCRC32 = 0,
        [Int32]$HeaderCRC32 = 0
    )

    $hdr = New-Object byte[] 512

    [Text.Encoding]::ASCII.GetBytes("EFI PART").CopyTo($hdr,0)
    [BitConverter]::GetBytes(0x00010000).CopyTo($hdr,8)
    [BitConverter]::GetBytes(92).CopyTo($hdr,12)

    # CRC32 placeholder (will be overwritten)
    [BitConverter]::GetBytes($HeaderCRC32).CopyTo($hdr,16)
    [BitConverter]::GetBytes([UINT32]0x0).CopyTo($hdr,20)

    [BitConverter]::GetBytes($HeaderLBA).CopyTo($hdr,24)
    [BitConverter]::GetBytes($BackupLBA).CopyTo($hdr,32)
    [BitConverter]::GetBytes($FirstUsableLBA).CopyTo($hdr,40)
    [BitConverter]::GetBytes($LastUsableLBA).CopyTo($hdr,48)

    $DiskGuid.ToByteArray().CopyTo($hdr,56)

    [BitConverter]::GetBytes($EntriesLBA).CopyTo($hdr,72)
    [BitConverter]::GetBytes($EntryCount).CopyTo($hdr,80)
    [BitConverter]::GetBytes($EntrySize).CopyTo($hdr,84)

    [BitConverter]::GetBytes($EntriesCRC32).CopyTo($hdr,88)

    return $hdr
}

function Write-GptEntries{
    param(
        [UInt64]$LBA,
        [array]$Parts,
        [int]$EntrySize,
        [int]$SectorSize
    )

    $ms = New-Object System.IO.MemoryStream
    $bwLocal = New-Object System.IO.BinaryWriter($ms)

    foreach($p in $Parts){
        $entry = New-Object byte[] $EntrySize

        ([Guid]$p.type).ToByteArray().CopyTo($entry,0)
        ([Guid]$p.unique).ToByteArray().CopyTo($entry,16)
        [BitConverter]::GetBytes([UInt64]$p.startLBA).CopyTo($entry,32)
        [BitConverter]::GetBytes([UInt64]$p.endLBA).CopyTo($entry,40)
        [BitConverter]::GetBytes([UInt64]$p.attributesValue).CopyTo($entry,48)

        $nameBytes = [Text.Encoding]::Unicode.GetBytes([string]$p.name)
        $maxNameBytes = $EntrySize - 56
        if ($nameBytes.Length -gt $maxNameBytes) {
            $nameBytes = $nameBytes[0..($maxNameBytes-1)]
        }
        $nameBytes.CopyTo($entry,56)

        $bwLocal.Write($entry)
    }

    $bwLocal.Flush()
    $bytes = $ms.ToArray()

    # # Write to disk
    # Seek-Bytes ($LBA * $SectorSize)
    # $fs.Write($bytes)
    # return ,$bytes

    # Pad to full entry array size (EntrySize * EntryCount)
    $expectedSize = $EntrySize * $Parts.Count
    $padded = New-Object byte[] $expectedSize
    $bytes.CopyTo($padded, 0)
    # Write to disk
    Seek-Bytes ($LBA * $SectorSize)
    $fs.Write($padded, 0, $padded.Length)
    # Return the exact bytes used for CRC
    return ,$padded
}

# ============================================================
# WRITE PRIMARY GPT
# ============================================================
$diskGuid = [Guid]::NewGuid()

$primaryHeaderLBA  = 1
$primaryEntriesLBA = 2

Log "Writing primary GPT header and entries"
# Generate GPT Entries
$primaryEntries = Write-GptEntries -LBA $primaryEntriesLBA -Parts $parts -EntrySize $entrySize -SectorSize $sectorSize
$entriesCRC = [Int32]((Compute-CRC32 $primaryEntries) -band 0xFFFFFFFF)

$primaryHdr = Write-GptHeader `
    -HeaderLBA $primaryHeaderLBA `
    -BackupLBA $backupHeaderLBA `
    -EntriesLBA $primaryEntriesLBA `
    -FirstUsableLBA $firstUsableLBA `
    -LastUsableLBA $lastUsableLBA `
    -EntryCount ([uint32]$entryCount) `
    -EntrySize ([uint32]$entrySize) `
    -DiskGuid $diskGuid `
    -EntriesCRC32 $entriesCRC `
    -HeaderCRC32 0

# Generate CRC32 Hash

# $headerCRC = [Int32]((Compute-CRC32 $primaryHdr[0..91]) -band 0xFFFFFFFF)
$hdrCopy = $primaryHdr.Clone()
$hdrCopy[16] = 0
$hdrCopy[17] = 0
$hdrCopy[18] = 0
$hdrCopy[19] = 0
$headerCRC = Compute-CRC32 $hdrCopy[0..91]

[BitConverter]::GetBytes($headerCRC).CopyTo($primaryHdr,16)

# Write Header
Seek-Bytes ($primaryHeaderLBA * $sectorSize)
$fs.Write([byte[]]$primaryHdr)

# ============================================================
# WRITE BACKUP GPT
# ============================================================
Log "Writing backup GPT header and entries"

# $backupEntries = Write-GptEntries -LBA $backupEntriesLBA -Parts $parts -EntrySize $entrySize -SectorSize $sectorSize
$backupEntries = Write-GptEntries -LBA $backupEntriesLBA -Parts $parts -EntrySize $entrySize -SectorSize $sectorSize

$backupEntriesCRC = [Int32]((Compute-CRC32 $backupEntries) -band 0xFFFFFFFF)

$backupHdr = Write-GptHeader `
    -HeaderLBA $backupHeaderLBA `
    -BackupLBA $primaryHeaderLBA `
    -EntriesLBA $backupEntriesLBA `
    -FirstUsableLBA $firstUsableLBA `
    -LastUsableLBA $lastUsableLBA `
    -EntryCount ([uint32]$entryCount) `
    -EntrySize ([uint32]$entrySize) `
    -DiskGuid $diskGuid `
    -EntriesCRC32 $backupEntriesCRC `
    -HeaderCRC32 0

$backupHeaderCRC = [Int32]((Compute-CRC32 $backupHdr[0..91]) -band 0xFFFFFFFF)
[BitConverter]::GetBytes($backupHeaderCRC).CopyTo($backupHdr,16)

Seek-Bytes ($backupHeaderLBA * $sectorSize)
$fs.Write([byte[]]$backupHdr)

# ============================================================
# DONE
# ============================================================
$fs.BaseStream.Close()

Log "Disk build complete"
Write-Host "Disk successfully built → $OutputImage"
