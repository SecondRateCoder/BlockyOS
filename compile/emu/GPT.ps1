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
      "size": 33554432,
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
    -LayoutJson   Path to JSON layout
    -OutputImage           Path to disk image or \\.\PhysicalDriveX
    -IsDrive               Write directly to a real drive (DANGEROUS)
    -LogFile               Log output
    -Verbose               Verbose logging
    -Help                  Show this help
    -ProtectiveMBR         File Path to a Protective MBR Binary

"@
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
    $mbr[510] = 0x55
    $mbr[511] = 0xAA

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

        # CHS start (set to 0/1/1)
        $Buffer[$Offset + 1] = 0x00
        $Buffer[$Offset + 2] = 0x02
        $Buffer[$Offset + 3] = 0x00

        # Type
        $Buffer[$Offset + 4] = $Type

        # CHS end (set to max: FF FF FF)
        $Buffer[$Offset + 5] = 0xFF
        $Buffer[$Offset + 6] = 0xFF
        $Buffer[$Offset + 7] = 0xFF

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
            sectors = $sectors
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
        $p = $computed[$i]
        Write-MbrEntry -Buffer $mbr -Offset ($entryOffset + 16 * $i) -Status 0x00 -Type 0xEE -StartLBA ([uint32]$p.start) -SectorCount ([uint32]$p.sectors)
    }

    return $mbr
}

if ($Help){Show-Help; exit}

# ============================================================
# LOGGING
# ============================================================
$logBuffer = @()

function Log{
    param([string]$Msg, [string]$Level = "INFO")

    $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $line = "[$ts] [$Level] $Msg"

    if($Verbose){Write-Host $line}
    if($LogFile){Add-Content -Path $LogFile -Value $line}

    $logBuffer += $line
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

    if (-not $Attributes) { return [uint64]0 }

    $attrs = @()
    if ($Attributes -is [string]) { $attrs = @($Attributes) }
    else { $attrs = @($Attributes) }

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
$backupEntriesLBA = $lastUsableLBA + 1
$backupHeaderLBA  = $backupEntriesLBA + $entrySectors
$totalSectors     = $backupHeaderLBA + 1

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
        $fs = New-Object System.IO.FileStream($OutputImage, [System.IO.FileMode]::Create, $access, [System.IO.FileShare]::None)
        $fs.SetLength([int64]($totalSectors * $sectorSize))
    }catch{
        Log "Failed to create disk image: $_" "ERROR"
        exit 1
    }
}

$bw = New-Object System.IO.BinaryWriter($fs)

function Seek-Bytes{
    param([UInt64]$Offset)
    $bw.BaseStream.Seek([int64]$Offset, [System.IO.SeekOrigin]::Begin) | Out-Null
}

# ============================================================
# WRITE PROTECTIVE MBR
# ============================================================
Log "Writing protective MBR"

$ProtectiveMBR = New-ProtectiveMBRFromJson -JsonPath $LayoutJson

# Partition entry 0: type EE, start LBA 1, size = min(totalSectors-1, 0xFFFFFFFF)
# CHS fields left as 0/FF (standard "large disk" encoding)
$ProtectiveMBR[446 + 4] = 0xEE
# [BitConverter]::GetBytes([uint32]1).CopyTo($ProtectiveMBR, 454)
# [BitConverter]::GetBytes([uint32][math]::Min($totalSectors - 1, 0xFFFFFFFF)).CopyTo($ProtectiveMBR, 458)
Seek-Bytes 0
$bw.Write($ProtectiveMBR)

# ============================================================
# GPT HEADER + ENTRIES HELPERS
# ============================================================
$diskGuid = [Guid]::NewGuid()

function Write-GptHeader{
    param(
        [UInt64]$HeaderLBA,
        [UInt64]$BackupLBA,
        [UInt64]$EntriesLBA,
        [UInt64]$FirstUsableLBA,
        [UInt64]$LastUsableLBA,
        [UInt32]$EntryCount,
        [UInt32]$EntrySize,
        [Guid]$DiskGuid
    )

    $hdr = New-Object byte[] 512

    # Signature "EFI PART"
    [Text.Encoding]::ASCII.GetBytes("EFI PART").CopyTo($hdr,0)

    # Revision 1.0
    [BitConverter]::GetBytes(0x00010000).CopyTo($hdr,8)

    # Header size
    [BitConverter]::GetBytes(92).CopyTo($hdr,12)

    # CRC32 (0 for now)
    # 16-19 reserved

    # Current LBA
    [BitConverter]::GetBytes($HeaderLBA).CopyTo($hdr,24)

    # Backup LBA
    [BitConverter]::GetBytes($BackupLBA).CopyTo($hdr,32)

    # First usable LBA
    [BitConverter]::GetBytes($FirstUsableLBA).CopyTo($hdr,40)

    # Last usable LBA
    [BitConverter]::GetBytes($LastUsableLBA).CopyTo($hdr,48)

    # Disk GUID
    $DiskGuid.ToByteArray().CopyTo($hdr,56)

    # Partition entries starting LBA
    [BitConverter]::GetBytes($EntriesLBA).CopyTo($hdr,72)

    # Number of partition entries
    [BitConverter]::GetBytes($EntryCount).CopyTo($hdr,80)

    # Size of each entry
    [BitConverter]::GetBytes($EntrySize).CopyTo($hdr,84)

    # CRC32 of partition array (0 for now)
    return $hdr
}

function Write-GptEntries{
    param(
        [UInt64]$LBA,
        [array]$Parts,
        [int]$EntrySize,
        [int]$SectorSize
    )

    Seek-Bytes ($LBA * $SectorSize)

    foreach($p in $Parts){
        $entry = New-Object byte[] $EntrySize

        # Type GUID
        ([Guid]$p.type).ToByteArray().CopyTo($entry,0)

        # Unique GUID
        ([Guid]::NewGuid()).ToByteArray().CopyTo($entry,16)

        # Start / End LBA
        [BitConverter]::GetBytes([UInt64]$p.startLBA).CopyTo($entry,32)
        [BitConverter]::GetBytes([UInt64]$p.endLBA).CopyTo($entry,40)

        # Attributes
        [BitConverter]::GetBytes([UInt64]$p.attributesValue).CopyTo($entry,48)

        # Name (UTF-16LE)
        $nameBytes = [Text.Encoding]::Unicode.GetBytes([string]$p.name)
        $maxNameBytes = $EntrySize - 56
        if($nameBytes.Length -gt $maxNameBytes){
            $nameBytes = $nameBytes[0..($maxNameBytes-1)]
        }
        $nameBytes.CopyTo($entry,56)

        $bw.Write($entry)
    }
}

# ============================================================
# WRITE PRIMARY GPT
# ============================================================
$primaryHeaderLBA  = 1
$primaryEntriesLBA = 2

Log "Writing primary GPT header and entries"

$primaryHdr = Write-GptHeader `
    -HeaderLBA $primaryHeaderLBA `
    -BackupLBA $backupHeaderLBA `
    -EntriesLBA $primaryEntriesLBA `
    -FirstUsableLBA $firstUsableLBA `
    -LastUsableLBA $lastUsableLBA `
    -EntryCount ([uint32]$entryCount) `
    -EntrySize ([uint32]$entrySize) `
    -DiskGuid $diskGuid

Seek-Bytes ($primaryHeaderLBA * $sectorSize)
$bw.Write([byte[]]$primaryHdr)

Write-GptEntries -LBA $primaryEntriesLBA -Parts $parts -EntrySize $entrySize -SectorSize $sectorSize

# ============================================================
# WRITE BACKUP GPT
# ============================================================
Log "Writing backup GPT header and entries"

Write-GptEntries -LBA $backupEntriesLBA -Parts $parts -EntrySize $entrySize -SectorSize $sectorSize

$backupHdr = Write-GptHeader `
    -HeaderLBA $backupHeaderLBA `
    -BackupLBA $primaryHeaderLBA `
    -EntriesLBA $backupEntriesLBA `
    -FirstUsableLBA $firstUsableLBA `
    -LastUsableLBA $lastUsableLBA `
    -EntryCount ([uint32]$entryCount) `
    -EntrySize ([uint32]$entrySize) `
    -DiskGuid $diskGuid

Seek-Bytes ($backupHeaderLBA * $sectorSize)
$bw.Write([byte[]]$backupHdr)

# ============================================================
# DONE
# ============================================================
$bw.Close()
$fs.Close()

Log "Disk build complete"
Write-Host "Disk successfully built → $OutputImage"
