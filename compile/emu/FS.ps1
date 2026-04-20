param(
    [string]$DiskImage,
    [string]$PartitionName,
    [string]$PartitionFlag,
    [string]$FileSystemType = "FAT32",
    [string]$VolumeName = "BlockyOS",
    [string]$BootCode = $null,
    [string]$SourceDirectory,
    [string]$LogFile,
    [switch]$Verbose,
    [switch]$Help
)

# ============================================================
# HELP
# ============================================================
function Show-Help {
@"
Partition File System Driver (FAT16 / FAT32 / NTFS)

SYNOPSIS
    Formats a single partition inside an existing disk image (GPT),
    selected by partition name and attribute flag, and optionally
    simulates loading a directory tree into it.

USAGE
    .\FSDriver.ps1 -DiskImage disk.img -PartitionName "EFI" -PartitionFlag "efi-boot" -FileSystemType FAT32 -SourceDirectory C:\Data -LogFile fs.log -Verbose

PARAMETERS
    -DiskImage       Path to existing disk image (with GPT)
    -PartitionName   GPT partition name to match
    -PartitionFlag   Attribute flag to match (e.g. "system", "efi-boot", "hidden")
    -FileSystemType  FAT16, FAT32, or NTFS (default FAT32)
    -SourceDirectory Directory to simulate loading into partition
    -LogFile         Path to log file
    -Verbose         Verbose console output
    -Help            Show this help

NOTES
    - Works on a partition-by-partition basis inside a disk image.
    - Selects partition by GPT name + attribute flag.
    - Writes only the boot sector; directory loading is simulated
      (size checks, warnings), not a full filesystem implementation.
"@
}

if ($Help) { Show-Help; exit }

# ============================================================
# LOGGING
# ============================================================
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

# ============================================================
# PARAM VALIDATION
# ============================================================
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

# ============================================================
# ATTRIBUTE FLAG → MASK
# ============================================================
function Get-GptAttributeMask {
    param([string]$Flag)

    switch ($Flag) {
        "system"         { return [uint64]0x0000000000000001 }
        "required"       { return [uint64]0x0000000000000001 }
        "efi-boot"       { return [uint64]0x0000000000000001 }
        "firmware-ignore"{ return [uint64]0x0000000000000002 }
        "legacy-boot"    { return [uint64]0x0000000000000004 }
        "readonly"       { return [uint64]0x0001000000000000 }
        "hidden"         { return [uint64]0x0002000000000000 }
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

# ============================================================
# GPT PARSING
# ============================================================
$sectorSize = 512

try {
    $fs = New-Object System.IO.FileStream($DiskImage, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::Read)
    $br = New-Object System.IO.BinaryReader($fs)
} catch {
    Log-Error "Could not open disk image: $_"
    exit 1
}

function Read-Sector {
    param([UInt64]$LBA)

    $fs.Seek([int64]($LBA * $sectorSize), [System.IO.SeekOrigin]::Begin) | Out-Null
    return $br.ReadBytes($sectorSize)
}

# Read GPT header at LBA 1
$gptHeader = Read-Sector 1
$signature = [Text.Encoding]::ASCII.GetString($gptHeader,0,8)

if ($signature -ne "EFI PART") {
    Log-Error "Disk image does not contain a valid GPT header at LBA 1 (signature: '$signature')"
    $br.Close()
    $fs.Close()
    exit 1
}

$entriesLBA   = [BitConverter]::ToUInt64($gptHeader,72)
$entryCount   = [BitConverter]::ToUInt32($gptHeader,80)
$entrySize    = [BitConverter]::ToUInt32($gptHeader,84)

Log-Message "GPT detected: EntriesLBA=$entriesLBA, EntryCount=$entryCount, EntrySize=$entrySize"

# Read GPT entries and find matching partition
$selectedPartition = $null

for ($i = 0; $i -lt $entryCount; $i++) {
    $entryOffsetBytes = ($entriesLBA * $sectorSize) + ($i * $entrySize)
    $fs.Seek([int64]$entryOffsetBytes, [System.IO.SeekOrigin]::Begin) | Out-Null
    $entry = $br.ReadBytes($entrySize)

    # Type GUID (16 bytes)
    $typeGuidBytes = $entry[0..15]
    $allZero = $true
    foreach($b in $typeGuidBytes){
        if($b -ne 0){$allZero = $false; break}
    }
    if($allZero){continue} # unused entry

    # Attributes (8 bytes at offset 48)
    $attr = [BitConverter]::ToUInt64($entry,48)

    # Name (UTF-16LE from offset 56)
    $nameBytes = $entry[56..($entrySize-1)]
    $name = [Text.Encoding]::Unicode.GetString($nameBytes).TrimEnd([char]0)

    if($name -ne $PartitionName){continue}

    if(($attr -band $flagMask) -eq 0){continue}

    # Start / End LBA
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
    $fs.Close()
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

# ============================================================
# BOOT SECTOR GENERATORS
# ============================================================
function Create-FAT32BootSector {
    param(
        [uint64]$PartitionSizeBytes,
        [string]$VolumeLabel = "BLOCKYOS",
        [uint32]$ClusterSize = 4096
    )

    $sectorSize = 512
    $sectorsPerCluster = [byte]($ClusterSize / $sectorSize)
    $reservedSectors = 32
    $fatCopies = 2
    $totalSectors = [uint32]($PartitionSizeBytes / $sectorSize)
    $fatSizeInSectors = [uint32][Math]::Ceiling(($totalSectors * 4.0) / ($sectorSize * 8.0))

    $bootSector = New-Object byte[] 512
    $ms = New-Object System.IO.MemoryStream($bootSector, 0, 512, $true)
    $writer = New-Object System.IO.BinaryWriter($ms)

    # Jump
    $writer.Write([byte]0xEB)
    $writer.Write([byte]0x3C)
    $writer.Write([byte]0x90)

    # OEM
    $oemBytes = [System.Text.Encoding]::ASCII.GetBytes("BLOCKYOS")
    $writer.Write($oemBytes, 0, 8)

    # Bytes per sector
    $writer.Write([uint16]$sectorSize)

    # Sectors per cluster
    $writer.Write([byte]$sectorsPerCluster)

    # Reserved sectors
    $writer.Write([uint16]$reservedSectors)

    # FATs
    $writer.Write([byte]$fatCopies)

    # Root entries (0 for FAT32)
    $writer.Write([uint16]0)

    # Total sectors (16-bit, 0 for FAT32)
    $writer.Write([uint16]0)

    # Media
    $writer.Write([byte]0xF8)

    # Sectors per FAT (16-bit, 0 for FAT32)
    $writer.Write([uint16]0)

    # Sectors per track
    $writer.Write([uint16]63)

    # Heads
    $writer.Write([uint16]255)

    # Hidden sectors
    $writer.Write([uint32]0)

    # Total sectors (32-bit)
    $writer.Write([uint32]$totalSectors)

    # FAT32 specific
    $writer.Write([uint32]$fatSizeInSectors) # FAT size
    $writer.Write([uint16]0)                 # Flags
    $writer.Write([uint16]0)                 # Version
    $writer.Write([uint32]2)                 # Root cluster
    $writer.Write([uint16]1)                 # FSInfo
    $writer.Write([uint16]6)                 # Backup boot sector

    # Reserved
    $writer.Write((New-Object byte[] 12),0,12)

    # Drive number
    $writer.Write([byte]0x80)
    # Reserved
    $writer.Write([byte]0)
    # Boot signature
    $writer.Write([byte]0x29)

    # Volume serial
    $writer.Write([uint32](Get-Random))

    # Volume label
    $labelBytes = [System.Text.Encoding]::ASCII.GetBytes($VolumeLabel)
    $writer.Write($labelBytes, 0, [Math]::Min($labelBytes.Length, 11))
    if($labelBytes.Length -lt 11){
        $writer.Write(([byte]0x20) * (11 - $labelBytes.Length))
    }

    # FS type
    $fsTypeBytes = [System.Text.Encoding]::ASCII.GetBytes("FAT32   ")
    $writer.Write($fsTypeBytes, 0, 8)

    # Boot code
    if($BootCode -eq $null){$writer.Write((Get-Content $BootCode), $writer.BaseStream.Position, 512 - ($writer.BaseStream.Position + 2))}
    else{$writer.Write((New-Object byte[] (512 - ($writer.BaseStream.Position + 2))), 0, 512 - ($writer.BaseStream.Position + 2))}
    

    # Signature
    $writer.Write([byte]0x55)
    $writer.Write([byte]0xAA)

    $writer.Flush()
    return $bootSector
}

function Create-FAT16BootSector {
    param(
        [uint64]$PartitionSizeBytes,
        [string]$VolumeLabel = "BLOCKYOS",
        [uint32]$ClusterSize = 4096
    )

    $sectorSize = 512
    $sectorsPerCluster = [byte]($ClusterSize / $sectorSize)
    $reservedSectors = 1
    $fatCopies = 2
    $totalSectors = [uint32]($PartitionSizeBytes / $sectorSize)
    $rootEntries = 512
    $fatSizeInSectors = [uint16][Math]::Ceiling(($totalSectors * 2.0) / ($sectorSize * 8.0))

    $bootSector = New-Object byte[] 512
    $ms = New-Object System.IO.MemoryStream($bootSector, 0, 512, $true)
    $writer = New-Object System.IO.BinaryWriter($ms)

    # Jump
    $writer.Write([byte]0xEB)
    $writer.Write([byte]0x3C)
    $writer.Write([byte]0x90)

    # OEM
    $oemBytes = [System.Text.Encoding]::ASCII.GetBytes("BLOCKYOS")
    $writer.Write($oemBytes,0,8)

    # Bytes per sector
    $writer.Write([uint16]$sectorSize)

    # Sectors per cluster
    $writer.Write([byte]$sectorsPerCluster)

    # Reserved sectors
    $writer.Write([uint16]$reservedSectors)

    # FATs
    $writer.Write([byte]$fatCopies)

    # Root entries
    $writer.Write([uint16]$rootEntries)

    # Total sectors (16-bit or 0)
    if ($totalSectors -le 0xFFFF) {
        $writer.Write([uint16]$totalSectors)
    } else {
        $writer.Write([uint16]0)
    }

    # Media
    $writer.Write([byte]0xF8)

    # Sectors per FAT
    $writer.Write([uint16]$fatSizeInSectors)

    # Sectors per track
    $writer.Write([uint16]63)

    # Heads
    $writer.Write([uint16]255)

    # Hidden sectors
    $writer.Write([uint32]0)

    # Total sectors (32-bit if needed)
    if($totalSectors -gt 0xFFFF){$writer.Write([uint32]$totalSectors)
    }else{$writer.Write([uint32]0)}

    # Drive number
    $writer.Write([byte]0x80)
    # Reserved
    $writer.Write([byte]0)
    # Boot signature
    $writer.Write([byte]0x29)

    # Volume serial
    $writer.Write([uint32](Get-Random))

    # Volume label
    $labelBytes = [System.Text.Encoding]::ASCII.GetBytes($VolumeLabel)
    $writer.Write($labelBytes,0,[Math]::Min($labelBytes.Length,11))
    if($labelBytes.Length -lt 11){
        $writer.Write(([byte]0x20) * (11 - $labelBytes.Length))
    }

    # FS type
    $fsTypeBytes = [System.Text.Encoding]::ASCII.GetBytes("FAT16   ")
    $writer.Write($fsTypeBytes,0,8)

    # Boot code
    # $writer.Write((New-Object byte[] 448),0,448)
    $writer.Write((Get-Content $BootCode), $writer.BaseStream.Position, 512 - ($writer.BaseStream.Position + 2))

    # Signature
    $writer.Write([byte]0x55)
    $writer.Write([byte]0xAA)

    $writer.Flush()
    return $bootSector
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
    $clustersPerMFTRecord = [sbyte]-10  # 2^10 = 1024 bytes

    $bootSector = New-Object byte[] 512
    $ms = New-Object System.IO.MemoryStream($bootSector, 0, 512, $true)
    $writer = New-Object System.IO.BinaryWriter($ms)

    # Jump
    $writer.Write([byte]0xEB)
    $writer.Write([byte]0x52)
    $writer.Write([byte]0x90)

    # OEM
    $oemBytes = [System.Text.Encoding]::ASCII.GetBytes("NTFS    ")
    $writer.Write($oemBytes,0,8)

    # Bytes per sector
    $writer.Write([uint16]$sectorSize)

    # Sectors per cluster
    $writer.Write([byte]$sectorsPerCluster)

    # Reserved sectors
    $writer.Write([uint16]0)

    # Always 0 for NTFS (FAT fields)
    $writer.Write([byte]0)
    $writer.Write([uint16]0)
    $writer.Write([uint16]0)
    $writer.Write([uint16]0)

    # Media
    $writer.Write([byte]0xF8)

    # More FAT fields (unused)
    $writer.Write([uint16]0)
    $writer.Write([uint16]0)
    $writer.Write([uint16]0)

    # Hidden sectors
    $writer.Write([uint32]0)

    # Total sectors (32-bit, 0 for NTFS)
    $writer.Write([uint32]0)

    # Total sectors (64-bit)
    $writer.Write([uint64]$totalSectors)

    # MFT / MFT mirror
    $writer.Write([uint64]$mftStartCluster)
    $writer.Write([uint64]$mft2StartCluster)

    # Clusters per MFT record
    $writer.Write([sbyte]$clustersPerMFTRecord)
    $writer.Write((New-Object byte[] 3),0,3)

    # Clusters per index block
    $writer.Write([byte]8)
    $writer.Write((New-Object byte[] 3),0,3)

    # Volume serial
    $writer.Write([uint64](Get-Random))

    # Checksum
    $writer.Write([uint32]0)

    # Bootstrap code
    $writer.Write((Get-Content $BootCode), $writer.BaseStream.Position, 512 - ($writer.BaseStream.Position + 2))

    # Signature
    $writer.Write([byte]0x55)
    $writer.Write([byte]0xAA)

    $writer.Flush()
    return $bootSector
}

# ---------- FAT directory loader (FAT16/32) ----------
function Write-FATDirectoryRecursive{
    param(
        [System.IO.FileStream]$Disk,
        [string]$HostPath,
        [uint32]$CurrentCluster,
        [uint32]$ClusterSize,
        [uint64]$DataStartOffset,
        [uint32[]]$FatTable
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
            if($Fat[$i] -eq 0){$Fat[$i] = 0x0FFFFFFF; return [uint32]$i}
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

    function New-DirEntry{
        param([string]$Name,[bool]$IsDir,[uint32]$StartCluster,[uint32]$Size)
        $e = New-Object byte[] 32
        $short = ($Name.ToUpper() -replace '[^A-Z0-9]', '_')
        if($short.Length -gt 8){$short = $short.Substring(0,8)}
        $short = $short.PadRight(8)
        $ext = ""
        if($Name.Contains(".")){
            $ext = $Name.Split(".")[-1].ToUpper()
            if ($ext.Length -gt 3) { $ext = $ext.Substring(0,3) }
        }
        $ext = $ext.PadRight(3)
        [Text.Encoding]::ASCII.GetBytes($short).CopyTo($e,0)
        [Text.Encoding]::ASCII.GetBytes($ext).CopyTo($e,8)
        $e[11] = $(if ($IsDir) { 0x10 } else { 0x20 })
        [BitConverter]::GetBytes([uint16]($StartCluster -shr 16)).CopyTo($e,20)
        [BitConverter]::GetBytes([uint16]($StartCluster -band 0xFFFF)).CopyTo($e,26)
        [BitConverter]::GetBytes($Size).CopyTo($e,28)
        return $e
    }

    $items = Get-ChildItem -LiteralPath $HostPath -Force
    $entries = New-Object System.Collections.Generic.List[byte[]]

    foreach ($item in $items) {
        if($item.Name -eq "." -or $item.Name -eq ".."){continue}

        if($item.PSIsContainer){
            $newCluster = Allocate-Cluster -Fat $FatTable
            $entries.Add( (New-DirEntry -Name $item.Name -IsDir $true -StartCluster $newCluster -Size 0) )
            Write-FATDirectoryRecursive -Disk $Disk -HostPath $item.FullName -CurrentCluster $newCluster -ClusterSize $ClusterSize -DataStartOffset $DataStartOffset -FatTable $FatTable
        }else{
            $bytes = [System.IO.File]::ReadAllBytes($item.FullName)
            $remaining = $bytes.Length
            $firstCluster = Allocate-Cluster -Fat $FatTable
            $cluster = $firstCluster
            $pos = 0
            while($remaining -gt 0){
                $chunk = [Math]::Min($ClusterSize,$remaining)
                $buf = New-Object byte[] $ClusterSize
                [Array]::Copy($bytes,$pos,$buf,0,$chunk)
                Write-Cluster -Cluster $cluster -Data $buf
                $remaining -= $chunk
                $pos += $chunk
                if($remaining -gt 0){$cluster = Append-Cluster -ChainStart $cluster -Fat $FatTable}
            }
            $entries.Add((New-DirEntry -Name $item.Name -IsDir $false -StartCluster $firstCluster -Size $bytes.Length))
        }
    }

    # Flatten entries into cluster(s)
    $dirBytes = New-Object System.IO.MemoryStream
    foreach ($e in $entries) { $dirBytes.Write($e,0,$e.Length) }
    $dirBytes.Flush()
    $data = $dirBytes.ToArray()
    $neededClusters = [uint32][Math]::Ceiling($data.Length / $ClusterSize)
    $cluster = $CurrentCluster
    $pos = 0
    for ($i=0; $i -lt $neededClusters; $i++) {
        $chunk = [Math]::Min($ClusterSize,$data.Length - $pos)
        $buf = New-Object byte[] $ClusterSize
        [Array]::Copy($data,$pos,$buf,0,$chunk)
        Write-Cluster -Cluster $cluster -Data $buf
        $pos += $chunk
        if ($i -lt $neededClusters-1) { $cluster = Append-Cluster -ChainStart $cluster -Fat $FatTable }
    }
}

# ---------- Format + load ----------
switch($FileSystemType){
    "FAT16" {
        Log-Message "Formatting as FAT16"
        $boot = New-FAT16BootSector -PartitionSizeBytes $partBytes
        $fs.Seek([int64]$partOffset,'Begin') | Out-Null
        $fs.Write($boot,0,$boot.Length)

        $clusterSize = 4096
        $clusters = [uint32]($partBytes / $clusterSize)
        $fatSizeBytes = $clusters * 2
        $fatSectors = [uint32][Math]::Ceiling($fatSizeBytes / $sectorSize)
        $fatTable = New-Object uint32[] $clusters  # store as 32-bit for convenience

        if ($SourceDirectory) {
            $dataOffset = $partOffset + (1 + (2 * $fatSectors) + (512 * 32 / $sectorSize)) * $sectorSize
            Write-FATDirectoryRecursive -Disk $fs -HostPath $SourceDirectory -CurrentCluster 2 -ClusterSize $clusterSize -DataStartOffset $dataOffset -FatTable $fatTable
        }
        # TODO: write FAT + root dir properly for FAT16 (left as exercise)
    } "FAT32" {
        Log-Message "Formatting as FAT32"
        $boot = Create-FAT32BootSector -PartitionSizeBytes $partBytes
        $fs.Seek([int64]$partOffset,'Begin') | Out-Null
        $fs.Write($boot,0,$boot.Length)

        $clusterSize = 4096
        $clusters = [uint32]($partBytes / $clusterSize)
        $fatSizeBytes = $clusters * 4
        $fatSectors = [uint32][Math]::Ceiling($fatSizeBytes / $sectorSize)
        $fatTable = New-Object uint32[] $clusters

        $reservedSectors = 32
        $fatStartOffset = $partOffset + ($reservedSectors * $sectorSize)
        $dataStartOffset = $fatStartOffset + (2 * $fatSectors * $sectorSize)

        if ($SourceDirectory) {
            Write-FATDirectoryRecursive -Disk $fs -HostPath $SourceDirectory -CurrentCluster 2 -ClusterSize $clusterSize -DataStartOffset $dataStartOffset -FatTable $fatTable
        }

        # Write FAT copies
        $fatBytes = New-Object byte[] ($fatSectors * $sectorSize)
        for ($i=0; $i -lt $fatTable.Length; $i++) {
            [BitConverter]::GetBytes($fatTable[$i]).CopyTo($fatBytes,$i*4)
        }
        for ($copy=0; $copy -lt 2; $copy++) {
            $off = $fatStartOffset + ($copy * $fatSectors * $sectorSize)
            $fs.Seek([int64]$off,'Begin') | Out-Null
            $fs.Write($fatBytes,0,$fatBytes.Length)
        }
    } "NTFS" {
        Log-Message "Formatting as NTFS (boot sector only; full NTFS loader not implemented)"
        $boot = New-NTFSBootSector -PartitionSizeBytes $partBytes
        $fs.Seek([int64]$partOffset,'Begin') | Out-Null
        $fs.Write($boot,0,$boot.Length)
        if ($SourceDirectory) {
            Log-Message "NTFS recursive loader not implemented – you’ll need a proper MFT/attribute implementation" "WARNING"
        }
    }
}

# ============================================================
# CLEANUP
# ============================================================
$br.Close()
$fs.Close()

Log-Message "Partition FS Driver Completed Successfully"
Write-Host "Partition formatted: $DiskImage (Partition '$PartitionName', Flag '$PartitionFlag', FS $FileSystemType)"
if ($Verbose -and $LogFile) {
    Write-Host "Operations logged to: $LogFile"
}
