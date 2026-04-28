[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [string]$LayoutJson,
    [Parameter(Mandatory=$true)]
    [string]$OutputImage,
    [string]$LogFile,
    [int]$SectorSize = 512,
    [switch]$IsDrive,
    [switch]$RepairOnly,
    [switch]$Help
)

# ============================================================
# HELP & CONSTANTS
# ============================================================
$MIN_GPT_ENTRIES = 128
$ENTRY_SIZE      = 128
$GPT_HDR_SIZE    = 92
$TABLE_SECTORS   = 32

function Show-Help {
@"
GPT/MBR Smart Builder & Repairer

SYNOPSIS
    Creates a GPT disk image (with protective MBR) or formats a real drive
    using a JSON partition layout. Enforces strict UEFI specification to 
    prevent gdisk warnings (CRC errors, overlaps, sizing gaps).

USAGE
    # Build disk image
    .\SmartGPT.ps1 -LayoutJson layout.json -OutputImage disk.img

    # Format physical drive (DANGEROUS)
    .\SmartGPT.ps1 -LayoutJson layout.json -OutputImage \\.\PhysicalDrive2 -IsDrive

    # Repair existing corrupted Image
    .\SmartGPT.ps1 -OutputImage corrupt.img -RepairOnly

JSON FORMAT
{
  "partitions": [
    {
      "name": "EFI",
      "size": 33554432,
      "type": "C12A7328-F81F-11D2-BA4B-00A0C93EC93B",
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
    -LayoutJson            Path to JSON layout
    -OutputImage           Path to disk image or \\.\PhysicalDriveX
    -IsDrive               Write directly to a real drive (DANGEROUS)
    -LogFile               Log output
    -Verbose               Verbose logging
    -RepairOnly            Skip build; recalculate and repair headers on an existing image
    -Help                  Show this help
"@
}

if ($Help) { Show-Help; exit }

# ============================================================
# LOGGING & CORE UTILS
# ============================================================
function Log {
    param([string]$Msg, [string]$Level = "INFO", [System.ConsoleColor]$color)
    $ts = Get-Date -Format "HH:mm:ss"
    $line = "[$ts] [$Level] $Msg"
    if($Verbose -or $Level -eq "ERROR"){
        if($color){Write-Host $line -ForegroundColor $color}else{Write-Host $line}
    }
    if($LogFile){ $line | Out-File -FilePath $LogFile -Append }
}

function Compute-CRC32 {
    param([byte[]]$Data)
    $table = New-Object uint32[] 256
    for ($i = 0; $i -lt 256; $i++) {
        $crc = [uint32]$i
        for ($j = 0; $j -lt 8; $j++) {
            if ($crc -band 1) { $crc = (0xEDB88320 -bxor ($crc -shr 1)) }
            else { $crc = ($crc -shr 1) }
        }
        $table[$i] = $crc
    }
    $crc32 = -bnot [uint32]0
    foreach ($b in $Data) {
        $idx = ($crc32 -bxor $b) -band 0xFF
        $crc32 = ($crc32 -shr 8) -bxor $table[$idx]
    }
    return [uint32](-bnot $crc32)
}

function Get-GptHeaderBytes {
    param($HdrLBA, $BackupLBA, $EntriesLBA, $FirstUsable, $LastUsable, $DiskGuid, $NumEntries, $EntrySize, $EntriesCRC)
    
    $hdr = New-Object byte[] 512
    [Text.Encoding]::ASCII.GetBytes("EFI PART").CopyTo($hdr, 0)
    [BitConverter]::GetBytes([uint32]0x00010000).CopyTo($hdr, 8)
    [BitConverter]::GetBytes([uint32]$GPT_HDR_SIZE).CopyTo($hdr, 12)
    [BitConverter]::GetBytes([uint64]$HdrLBA).CopyTo($hdr, 24)
    [BitConverter]::GetBytes([uint64]$BackupLBA).CopyTo($hdr, 32)
    [BitConverter]::GetBytes([uint64]$FirstUsable).CopyTo($hdr, 40)
    [BitConverter]::GetBytes([uint64]$LastUsable).CopyTo($hdr, 48)
    $DiskGuid.ToByteArray().CopyTo($hdr, 56)
    [BitConverter]::GetBytes([uint64]$EntriesLBA).CopyTo($hdr, 72)
    [BitConverter]::GetBytes([uint32]$NumEntries).CopyTo($hdr, 80)
    [BitConverter]::GetBytes([uint32]$EntrySize).CopyTo($hdr, 84)
    [BitConverter]::GetBytes([uint32]$EntriesCRC).CopyTo($hdr, 88)
    
    $hdr[16..19] = 0,0,0,0
    $crc = Compute-CRC32 $hdr[0..($GPT_HDR_SIZE-1)]
    [BitConverter]::GetBytes([uint32]$crc).CopyTo($hdr, 16)
    return $hdr
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

        Log -Info "WARNING" -color Yellow ">> [WRITE] Position=$abs  Count=$count  Caller=$caller"

        return $this.BaseStream.Write($buffer, $offset, $count)
    }

    $proxy | Add-Member -MemberType ScriptMethod -Name SetLength -Value {
        param($count)

        Log -Info "WARNING" -color Yellow ">> [SET-LENGTH] New=$count"

        return $this.BaseStream.SetLength($count)
    }

    $proxy | Add-Member -MemberType ScriptMethod -Name Dispose -Value {
        param()

        Log -Info "WARNING" -color Yellow ">> [DISPOSING]"

        return $this.BaseStream.SetLength($count)
    }

    return $proxy
}

# ============================================================
# JSON PARSING & ATTRIBUTES
# ============================================================
function Get-GptAttributeValue {
    param([object]$Attributes)
    if(-not $Attributes){return [uint64]0}

    $attrs = if($Attributes -is [string]){ @($Attributes) } else { @($Attributes) }
    [uint64]$value = 0

    foreach($a in $attrs){
        switch($a){
            "system"          { $value = $value -bor 0x0000000000000001UL }
            "required"        { $value = $value -bor 0x0000000000000001UL }
            "efi-boot"        { $value = $value -bor 0x0000000000000001UL }
            "firmware-ignore" { $value = $value -bor 0x0000000000000002UL }
            "legacy-boot"     { $value = $value -bor 0x0000000000000004UL }
            "readonly"        { $value = $value -bor 0x0001000000000000UL }
            "hidden"          { $value = $value -bor 0x0002000000000000UL }
            default {
                if($a -match "^0x[0-9A-Fa-f]+$"){
                    $value = $value -bor ([UInt64]::Parse($a.Substring(2), "HexNumber"))
                }
            }
        }
    }
    return $value
}

function Normalize-Give {
    param([object]$Give)
    if(-not $Give){return @()}
    if($Give -is [string]){return @($Give)}
    return @($Give)
}

function Apply-GiveDirectives {
    param([array]$Parts, [uint64]$UsableSectors, [uint64]$FirstUsableLBA)

    [uint64]$totalRequested = ($Parts | Measure-Object -Property sectors -Sum).Sum

    # FIT Logic
    if($totalRequested -gt $UsableSectors){
        [uint64]$overflow = $totalRequested - $UsableSectors
        foreach($p in ($Parts | Where-Object { $_.give -contains "fit" })){
            if($overflow -le 0){break}
            [uint64]$canReduce = if ($p.sectors -gt 1) { $p.sectors - 1 } else { 0 }
            if ($canReduce -le 0) { continue }
            
            [uint64]$reduceBy = [math]::Min($canReduce, $overflow)
            $p.sectors -= $reduceBy
            $overflow  -= $reduceBy
        }
        if($overflow -gt 0){
            Log "Unable to fit partitions within disk using 'fit' directives." "ERROR" -color Red
            exit 1
        }
    }

    # GROW Logic
    $growParts = $Parts | Where-Object{$_.give -contains "grow"}
    if($growParts.Count -gt 1){
        Log "Multiple 'grow' partitions detected — only one allowed." "ERROR" -color Red
        exit 1
    }

    [uint64]$totalAfterFit = ($Parts | Measure-Object -Property sectors -Sum).Sum
    [uint64]$remaining     = $UsableSectors - $totalAfterFit

    if($growParts.Count -eq 1 -and $remaining -gt 0){
        $growParts[0].sectors += $remaining
    }

    # Assign sequential LBAs
    # Note: We push the first partition to 2048 for physical alignment, 
    # even though FirstUsableLBA technically starts at 34 in the header.
    [uint64]$cur = [math]::Max($FirstUsableLBA, 2048)
    foreach($p in $Parts){
        $p | Add-Member -MemberType NoteProperty -Name "startLBA" -Value $cur -Force
        $p | Add-Member -MemberType NoteProperty -Name "endLBA" -Value ($cur + $p.sectors - 1) -Force
        $cur = $p.endLBA + 1
    }
}

# ============================================================
# SEALER & REPAIR ENGINE (gdisk Compliance Fixer)
# ============================================================
function Invoke-DeepProbeAndRepair {
    param($Stream, $SectorSize)
    
    Log "Probing for UEFI/gdisk compliance..." "PROBE" -color Cyan
    $totalSectors = [int64]($Stream.Length / $SectorSize)
    
    # Preserve GUID if it exists, otherwise make a new one
    $Stream.Seek($SectorSize, 'Begin') | Out-Null
    $pBuf = New-Object byte[] $SectorSize
    $Stream.Read($pBuf, 0, $SectorSize) | Out-Null
    $diskGuid = [Guid]::new($pBuf[56..71])
    if ($diskGuid -eq [Guid]::Empty) { $diskGuid = [Guid]::NewGuid() }

    # Strict UEFI Boundaries
    $numEntries = $MIN_GPT_ENTRIES
    $entrySize = $ENTRY_SIZE
    $primaryEntriesLBA = 2
    $firstUsable = 34  # Fixes metadata gap warning (even if partitions start at 2048)
    
    $backupHeaderLBA = $totalSectors - 1
    $backupEntriesLBA = $backupHeaderLBA - $TABLE_SECTORS
    $lastUsable = $backupEntriesLBA - 1 # Fixes overlap warning

    # Calculate CRC on exactly 16KB of entries (Fixes Table Size warning)
    $Stream.Seek($primaryEntriesLBA * $SectorSize, 'Begin') | Out-Null
    $rawEntries = New-Object byte[] ($numEntries * $entrySize)
    $Stream.Read($rawEntries, 0, $rawEntries.Length) | Out-Null
    $calcCRC = Compute-CRC32 $rawEntries

    # Seal Primary
    $newPrimary = Get-GptHeaderBytes 1 $backupHeaderLBA $primaryEntriesLBA $firstUsable $lastUsable $diskGuid $numEntries $entrySize $calcCRC
    $Stream.Seek($SectorSize, 'Begin') | Out-Null
    $Stream.Write($newPrimary, 0, $SectorSize)

    # Seal Backup Table & Header
    $Stream.Seek($backupEntriesLBA * $SectorSize, 'Begin') | Out-Null
    $Stream.Write($rawEntries, 0, $rawEntries.Length)
    
    $newBackup = Get-GptHeaderBytes $backupHeaderLBA 1 $backupEntriesLBA $firstUsable $lastUsable $diskGuid $numEntries $entrySize $calcCRC
    $Stream.Seek($backupHeaderLBA * $SectorSize, 'Begin') | Out-Null
    $Stream.Write($newBackup, 0, $SectorSize)
    
    Log "Disk structure sealed successfully." "SUCCESS" -color Green
}

# ============================================================
# MAIN EXECUTION
# ============================================================
if ($RepairOnly) {
    $fs = New-LoggedFileStream -Path $OutputImage
    if(-not(Test-Path $OutputImage)){throw "Image not found."}
    try{Invoke-DeepProbeAndRepair $fs $SectorSize}finally{$fs.Dispose()}
    exit
}

if (-not $LayoutJson) { throw "LayoutJson is required for Build mode." }

Log "Parsing JSON Layout..."
$layout = Get-Content $LayoutJson -Raw | ConvertFrom-Json
$parts = $layout.partitions

# Phase 1: Property Normalization
foreach($p in $parts){
    $p | Add-Member -MemberType NoteProperty -Name "give" -Value (Normalize-Give $p.give) -Force
    $p | Add-Member -MemberType NoteProperty -Name "attributesValue" -Value (Get-GptAttributeValue $p.attributes) -Force
    $p | Add-Member -MemberType NoteProperty -Name "sectors" -Value ([math]::Ceiling($p.size / $SectorSize)) -Force
}

# Phase 2: Sizing & File Creation
$requestedDataSectors = ($parts | Measure-Object -Property sectors -Sum).Sum

if ($IsDrive) {
    Log "Opening Physical Drive: $OutputImage" "INFO" -color Yellow
    $fs = [System.IO.File]::Open($OutputImage, 'Open', 'ReadWrite')
    $totalSectors = [int64]($fs.Length / $SectorSize)
}else{
    Log "Creating Disk Image: $OutputImage"
    # 34 sectors for primary structures, 33 for backup structures, plus 2014 padding if starting at 2048
    $totalSectors = 34 + 33 + 2014 + $requestedDataSectors
    $fs = New-LoggedFileStream -Path $OutputImage
    $fs.SetLength($totalSectors * $SectorSize)
}

$layout = Get-Content $LayoutJson -Raw | ConvertFrom-Json
$parts = $layout.partitions

# Initial LBA Calculation
[uint64]$currentLBA = 2048
foreach($p in $parts) {
    [uint64]$pSectors = [math]::Ceiling($p.size / $SectorSize)
    $p | Add-Member -MemberType NoteProperty -Name "startLBA" -Value $currentLBA -Force
    $p | Add-Member -MemberType NoteProperty -Name "endLBA" -Value ($currentLBA + $pSectors - 1) -Force
    $p | Add-Member -MemberType NoteProperty -Name "attrVal" -Value (Get-GptAttributeValue $p.attributes) -Force
    $currentLBA = $p.endLBA + 1
}

$totalSectors = [int64]($currentLBA + $TABLE_SECTORS + 1)
$fs = New-LoggedFileStream -Path $OutputImage
try {
    $fs.SetLength($totalSectors * $SectorSize)

    # PROTECTIVE MBR (STRICT FIX)
    $mbr = New-Object byte[] 512
    $mbr[510] = 0x55; $mbr[511] = 0xAA
    $mbr[446+4] = 0xEE # Type
    [BitConverter]::GetBytes([uint32]1).CopyTo($mbr, 446+8) # Start LBA
    
    # Calculate MBR Size safely using [int64] first
    [int64]$rawMbrSize = $totalSectors - 1
    [uint32]$safeMbrSize = 0
    if ($rawMbrSize -gt 0) {
        # Cap at 0xFFFFFFFF (Max UInt32)
        if ($rawMbrSize -gt 4294967295) { $safeMbrSize = 4294967295 }
        else { $safeMbrSize = [uint32]$rawMbrSize }
    }
    [BitConverter]::GetBytes($safeMbrSize).CopyTo($mbr, 446+12)
    $fs.Write($mbr, 0, 512)

    # GPT ENTRY TABLE
    $tableBuf = New-Object byte[] ($MIN_GPT_ENTRIES * $ENTRY_SIZE)
    for($i=0; $i -lt $parts.Count; $i++) {
        $p = $parts[$i]; $off = $i * $ENTRY_SIZE
        ([Guid]$p.type).ToByteArray().CopyTo($tableBuf, $off)
        [Guid]::NewGuid().ToByteArray().CopyTo($tableBuf, $off+16)
        [BitConverter]::GetBytes([uint64]$p.startLBA).CopyTo($tableBuf, $off+32)
        [BitConverter]::GetBytes([uint64]$p.endLBA).CopyTo($tableBuf, $off+40)
        [BitConverter]::GetBytes([uint64]$p.attrVal).CopyTo($tableBuf, $off+48)
        [System.Text.Encoding]::Unicode.GetBytes($p.name).CopyTo($tableBuf, $off+56)
    }
    $fs.Seek(2 * $SectorSize, 'Begin') | Out-Null
    $fs.Write($tableBuf, 0, $tableBuf.Length)

    Invoke-GptSeal $fs $SectorSize
} finally {
    $fs.Dispose()
    Log "Build complete."
}