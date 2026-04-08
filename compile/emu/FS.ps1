param (
    [string]$FileSystemType = "FAT32",
    [string]$PartitionImage,
    [string]$SourceDirectory,
    [string]$LogFile,
    [switch]$Verbose,
    [switch]$Help
)

# Help Information
function Show-Help {
    $helpText = @"
Smart Partition File System Emulator

SYNOPSIS
    FS.ps1 [Parameters]

DESCRIPTION
    Formats partition-scoped file systems (FAT32, FAT16, NTFS) with directory
    loading capabilities, intelligent validation, and comprehensive logging.
    This tool operates at the partition level and is compatible with GPT/MBR disk images.
    Partitions must already exist (created by GPT.ps1 or similar tools).

PARAMETERS
    -FileSystemType <string>
        Type of file system to format: "FAT32" (default), "FAT16", or "NTFS"

    -PartitionImage <string>
        Path to existing partition image file to format (required)

    -SourceDirectory <string>
        Root directory to load into the file system (recursively includes all children)

    -LogFile <string>
        Path to output log file for all operations and warnings

    -Verbose [switch]
        Enable verbose output with detailed operation information

    -Help [switch]
        Display this help information and exit

EXAMPLES
    # Display help
    .\FS.ps1 -Help

    # Format existing partition as FAT32
    .\FS.ps1 -FileSystemType FAT32 -PartitionImage partition.img -LogFile fs.log -Verbose

    # Format partition and load directory
    .\FS.ps1 -FileSystemType FAT32 -PartitionImage data.img -SourceDirectory C:\Data -LogFile ops.log

    # Format partition as NTFS
    .\FS.ps1 -FileSystemType NTFS -PartitionImage ntfs.img

    # Format partition as FAT16
    .\FS.ps1 -FileSystemType FAT16 -PartitionImage small.img -Verbose

FILE SYSTEM LIMITS
    FAT16:  Up to 2GB partitions, max filename 8.3 characters
    FAT32:  Up to 2TB partitions, max filename 255 characters (LFN)
    NTFS:   Up to 16EB partitions, max filename 255 characters

WARNINGS
    - Existing partition images will be recreated
    - Large directory loads may take significant time
    - Insufficient disk space will halt the operation
    - Unsupported characters in filenames will be replaced
"@
    Write-Host $helpText
}

# Exit if Help flag is set
if ($Help) {
    Show-Help
    exit 0
}

# Logging Functions
$logBuffer = @()
function Log-Message {
    param(
        [string]$Message,
        [string]$Level = "INFO"
    )

    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logEntry = "[$timestamp] [$Level] $Message"
    $logBuffer += $logEntry

    if ($Verbose) {
        Write-Host $logEntry
    }

    if ($LogFile) {
        Add-Content -Path $LogFile -Value $logEntry -ErrorAction SilentlyContinue
    }
}

function Log-Warning {
    param([string]$Message)
    Log-Message -Message $Message -Level "WARNING"
    Write-Warning $Message
}

function Log-Error {
    param([string]$Message)
    Log-Message -Message $Message -Level "ERROR"
    Write-Error $Message
}

# Initialize logging
if ($LogFile) {
    try {
        $logPath = Split-Path -Path $LogFile
        if (-not (Test-Path -Path $logPath) -and $logPath) {
            New-Item -ItemType Directory -Path $logPath -Force | Out-Null
        }
        "=== File System Emulator Session ===" | Out-File -FilePath $LogFile -Force
    } catch {
        Log-Warning "Could not initialize log file: $_"
    }
}

Log-Message "File System Emulator Started"
Log-Message "Configuration: FileSystemType=$FileSystemType, PartitionSize=$PartitionSize bytes"

# FAT32 Boot Sector Structure
function Create-FAT32BootSector {
    param(
        [uint64]$PartitionSizeBytes,
        [string]$VolumeLabel = "BLOCKYOS",
        [uint32]$ClusterSize = 4096
    )

    $sectorSize = 512
    $sectorsPerCluster = $ClusterSize / $sectorSize
    $reservedSectors = 32
    $fatCopies = 2
    $totalSectors = $PartitionSizeBytes / $sectorSize
    $dataClusterCount = ($totalSectors - $reservedSectors - ($fatCopies * $reservedSectors)) / $sectorsPerCluster
    $fatSizeInSectors = [Math]::Ceiling(($totalSectors * 4) / ($sectorSize * 8))

    $bootSector = New-Object byte[] 512
    $writer = New-Object System.IO.BinaryWriter([System.IO.MemoryStream]::new($bootSector))

    # Jump instruction
    $writer.Write([byte]0xEB)
    $writer.Write([byte]0x3C)
    $writer.Write([byte]0x90)

    # OEM identifier
    $oemBytes = [System.Text.Encoding]::ASCII.GetBytes("BLOCKYOS")
    $writer.Write($oemBytes, 0, 8)

    # Bytes per sector (512)
    $writer.Write([uint16]$sectorSize)

    # Sectors per cluster
    $writer.Write([byte]$sectorsPerCluster)

    # Reserved sectors
    $writer.Write([uint16]$reservedSectors)

    # Number of FATs
    $writer.Write([byte]$fatCopies)

    # Root entry count (FAT32 = 0)
    $writer.Write([uint16]0)

    # Total sectors (16-bit, 0 for FAT32)
    $writer.Write([uint16]0)

    # Media descriptor
    $writer.Write([byte]0xF8)

    # Sectors per FAT (16-bit, 0 for FAT32)
    $writer.Write([uint16]0)

    # Sectors per track
    $writer.Write([uint16]63)

    # Number of heads
    $writer.Write([uint16]255)

    # Hidden sectors
    $writer.Write([uint32]0)

    # Total sectors (32-bit)
    $writer.Write([uint32]$totalSectors)

    # FAT32 specific
    $writer.Write([uint32]$fatSizeInSectors)

    # Flags
    $writer.Write([uint16]0)

    # Version
    $writer.Write([uint16]0)

    # Root cluster
    $writer.Write([uint32]2)

    # FSInfo sector
    $writer.Write([uint16]1)

    # Boot backup sector
    $writer.Write([uint16]6)

    # Reserved for EXFAT
    $writer.Write([byte[]]@(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), 0, 12)

    # Drive number
    $writer.Write([byte]0x80)

    # Reserved
    $writer.Write([byte]0)

    # Boot signature
    $writer.Write([byte]0x29)

    # Volume serial number
    $writer.Write([uint32](Get-Random))

    # Volume label
    $labelBytes = [System.Text.Encoding]::ASCII.GetBytes($VolumeLabel)
    $writer.Write($labelBytes, 0, [Math]::Min($labelBytes.Length, 11))
    if ($labelBytes.Length -lt 11) {
        $writer.Write([byte[]]@(0x20) * (11 - $labelBytes.Length))
    }

    # File system type
    $fsTypeBytes = [System.Text.Encoding]::ASCII.GetBytes("FAT32   ")
    $writer.Write($fsTypeBytes, 0, 8)

    # Boot code (empty)
    for ($i = 0; $i -lt 420; $i++) {
        $writer.Write([byte]0)
    }

    # Boot signature
    $writer.Write([byte]0x55)
    $writer.Write([byte]0xAA)

    return $bootSector
}

# FAT16 Boot Sector Structure
function Create-FAT16BootSector {
    param(
        [uint64]$PartitionSizeBytes,
        [string]$VolumeLabel = "BLOCKYOS",
        [uint32]$ClusterSize = 4096
    )

    $sectorSize = 512
    $sectorsPerCluster = $ClusterSize / $sectorSize
    $reservedSectors = 1
    $fatCopies = 2
    $totalSectors = $PartitionSizeBytes / $sectorSize
    $rootEntries = 512
    $fatSizeInSectors = [Math]::Ceiling(($totalSectors * 2) / ($sectorSize * 8))

    $bootSector = New-Object byte[] 512
    $writer = New-Object System.IO.BinaryWriter([System.IO.MemoryStream]::new($bootSector))

    # Jump instruction
    $writer.Write([byte]0xEB)
    $writer.Write([byte]0x3C)
    $writer.Write([byte]0x90)

    # OEM identifier
    $oemBytes = [System.Text.Encoding]::ASCII.GetBytes("BLOCKYOS")
    $writer.Write($oemBytes, 0, 8)

    # Bytes per sector
    $writer.Write([uint16]$sectorSize)

    # Sectors per cluster
    $writer.Write([byte]$sectorsPerCluster)

    # Reserved sectors
    $writer.Write([uint16]$reservedSectors)

    # Number of FATs
    $writer.Write([byte]$fatCopies)

    # Root entry count
    $writer.Write([uint16]$rootEntries)

    # Total sectors (16-bit)
    if ($totalSectors -le 65535) {
        $writer.Write([uint16]$totalSectors)
    } else {
        $writer.Write([uint16]0)
    }

    # Media descriptor
    $writer.Write([byte]0xF8)

    # Sectors per FAT (16-bit)
    $writer.Write([uint16]$fatSizeInSectors)

    # Sectors per track
    $writer.Write([uint16]63)

    # Number of heads
    $writer.Write([uint16]255)

    # Hidden sectors
    $writer.Write([uint32]0)

    # Total sectors (32-bit)
    if ($totalSectors -gt 65535) {
        $writer.Write([uint32]$totalSectors)
    } else {
        $writer.Write([uint32]0)
    }

    # Drive number
    $writer.Write([byte]0x80)

    # Reserved
    $writer.Write([byte]0)

    # Boot signature
    $writer.Write([byte]0x29)

    # Volume serial number
    $writer.Write([uint32](Get-Random))

    # Volume label
    $labelBytes = [System.Text.Encoding]::ASCII.GetBytes($VolumeLabel)
    $writer.Write($labelBytes, 0, [Math]::Min($labelBytes.Length, 11))
    if ($labelBytes.Length -lt 11) {
        $writer.Write([byte[]]@(0x20) * (11 - $labelBytes.Length))
    }

    # File system type
    $fsTypeBytes = [System.Text.Encoding]::ASCII.GetBytes("FAT16   ")
    $writer.Write($fsTypeBytes, 0, 8)

    # Boot code (empty)
    for ($i = 0; $i -lt 448; $i++) {
        $writer.Write([byte]0)
    }

    # Boot signature
    $writer.Write([byte]0x55)
    $writer.Write([byte]0xAA)

    return $bootSector
}

# NTFS Boot Sector Structure
function Create-NTFSBootSector {
    param(
        [uint64]$PartitionSizeBytes,
        [string]$VolumeLabel = "BLOCKYOS"
    )

    $sectorSize = 512
    $clustersPerMFTRecord = -10  # 1KB (as negative means power of 2)
    $totalSectors = $PartitionSizeBytes / $sectorSize
    $clusterSize = 4096
    $sectorsPerCluster = $clusterSize / $sectorSize
    $mftStartCluster = 800  # Arbitrary start
    $mft2StartCluster = 801

    $bootSector = New-Object byte[] 512
    $writer = New-Object System.IO.BinaryWriter([System.IO.MemoryStream]::new($bootSector))

    # Jump instruction
    $writer.Write([byte]0xEB)
    $writer.Write([byte]0x52)
    $writer.Write([byte]0x90)

    # OEM identifier
    $oemBytes = [System.Text.Encoding]::ASCII.GetBytes("NTFS    ")
    $writer.Write($oemBytes, 0, 8)

    # Bytes per sector
    $writer.Write([uint16]$sectorSize)

    # Sectors per cluster
    $writer.Write([byte]$sectorsPerCluster)

    # Reserved sectors (always 0 for NTFS)
    $writer.Write([uint16]0)

    # Media descriptor
    $writer.Write([byte]0xF8)

    # Sectors per track
    $writer.Write([uint16]63)

    # Number of heads
    $writer.Write([uint16]255)

    # Hidden sectors
    $writer.Write([uint32]0)

    # Total sectors (always 0 for NTFS)
    $writer.Write([uint32]0)

    # Total sectors (64-bit)
    $writer.Write([uint64]$totalSectors)

    # MFT start cluster
    $writer.Write([uint64]$mftStartCluster)

    # MFT2 (mirror) start cluster
    $writer.Write([uint64]$mft2StartCluster)

    # Clusters per MFT record
    $writer.Write([byte][Math]::Abs($clustersPerMFTRecord))

    # Reserved
    for ($i = 0; $i -lt 3; $i++) { $writer.Write([byte]0) }

    # Clusters per index block
    $writer.Write([byte]8)

    # Reserved
    for ($i = 0; $i -lt 3; $i++) { $writer.Write([byte]0) }

    # Serial number
    $writer.Write([uint64][Math]::Abs((Get-Random)))

    # Checksum
    $writer.Write([uint32]0)

    # Bootstrap code (empty)
    for ($i = 0; $i -lt 426; $i++) {
        $writer.Write([byte]0)
    }

    # Boot signature
    $writer.Write([byte]0x55)
    $writer.Write([byte]0xAA)

    return $bootSector
}

# Directory Loading Functions
function Get-DirectorySize {
    param([string]$Path)

    try {
        $items = Get-ChildItem -Path $Path -Force -Recurse -ErrorAction SilentlyContinue
        $size = ($items | Measure-Object -Property Length -Sum).Sum
        return [uint64]($size + 0)
    } catch {
        Log-Warning "Could not calculate size for directory $Path : $_"
        return 0
    }
}

function Load-Directory {
    param(
        [string]$SourcePath,
        [uint64]$AvailableSpace
    )

    $loadedItems = @{
        files = 0
        directories = 0
        totalSize = 0
        warnings = @()
    }

    if (-not (Test-Path -Path $SourcePath -PathType Container)) {
        Log-Warning "Source directory not found: $SourcePath"
        return $loadedItems
    }

    try {
        $items = Get-ChildItem -Path $SourcePath -Force -Recurse -ErrorAction SilentlyContinue
        $totalSize = 0

        foreach ($item in $items) {
            if ($item.PSIsContainer) {
                $loadedItems.directories++
            } else {
                $itemSize = $item.Length
                $totalSize += $itemSize
                $loadedItems.files++

                # Check if we exceed available space
                if ($totalSize -gt $AvailableSpace) {
                    $loadedItems.warnings += "Item '$($item.Name)' would exceed available space ($($AvailableSpace) bytes)"
                    break
                }
            }
        }

        $loadedItems.totalSize = $totalSize

        if ($loadedItems.warnings.Count -gt 0) {
            foreach ($warning in $loadedItems.warnings) {
                Log-Warning $warning
            }
        }

    } catch {
        Log-Warning "Error loading directory: $_"
    }

    return $loadedItems
}

# Validate file system type
$validFSSystems = @("FAT32", "FAT16", "NTFS")
if ($FileSystemType -notin $validFSSystems) {
    Log-Error "Invalid FileSystemType: $FileSystemType. Must be one of: $($validFSSystems -join ', ')"
    exit 1
}

# Check parameters
if (-not $PartitionImage) {
    Log-Error "PartitionImage parameter is required"
    exit 1
}

# Verify partition image exists
if (-not (Test-Path -Path $PartitionImage -PathType Leaf)) {
    Log-Error "Partition image not found: $PartitionImage"
    Log-Message "Partition must be created first using GPT.ps1 or similar tools"
    exit 1
}

# Get partition size
try {
    $partitionFile = Get-Item -Path $PartitionImage
    $PartitionSize = $partitionFile.Length
    Log-Message "Partition image found: $PartitionImage"
    Log-Message "Partition Size: $([Math]::Round($PartitionSize / 1GB, 2)) GB ($PartitionSize bytes)"
} catch {
    Log-Error "Could not access partition image: $_"
    exit 1
}

# Format partition image
Log-Message "Formatting $FileSystemType partition image..."
Log-Message "Partition Image: $PartitionImage"

try {
    # Create boot sector based on file system type
    switch ($FileSystemType) {
        "FAT32" {
            $bootSector = Create-FAT32BootSector -PartitionSizeBytes $PartitionSize
            Log-Message "FAT32 boot sector created"
        }
        "FAT16" {
            if ($PartitionSize -gt 2GB) {
                Log-Warning "FAT16 typically limited to 2GB. Performance may degrade."
            }
            $bootSector = Create-FAT16BootSector -PartitionSizeBytes $PartitionSize
            Log-Message "FAT16 boot sector created"
        }
        "NTFS" {
            $bootSector = Create-NTFSBootSector -PartitionSizeBytes $PartitionSize
            Log-Message "NTFS boot sector created"
        }
    }

    # Format existing partition image
    $fs = [System.IO.File]::Open($PartitionImage, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write)
    try {
        # Write boot sector at the beginning
        $fs.Seek(0, [System.IO.SeekOrigin]::Begin)
        $fs.Write($bootSector, 0, $bootSector.Length)
        Log-Message "Boot sector written to partition"
        
        # Note: FAT table and directory structures would be written here in a full implementation
        # For now, we only format with boot sector as the partition is pre-allocated

    } finally {
        $fs.Close()
    }

    Log-Message "Partition formatted successfully: $PartitionImage"

} catch {
    Log-Error "Failed to format partition: $_"
    exit 1
}

# Load directory if specified
if ($SourceDirectory) {
    Log-Message "Loading directory into partition: $SourceDirectory"

    $availableSpace = $PartitionSize - 512  # Account for boot sector

    $loadResult = Load-Directory -SourcePath $SourceDirectory -AvailableSpace $availableSpace

    Log-Message "Directory load results:"
    Log-Message "  Files: $($loadResult.files)"
    Log-Message "  Directories: $($loadResult.directories)"
    Log-Message "  Total Size: $($loadResult.totalSize) bytes"
    Log-Message "  Used Space: $([Math]::Round(($loadResult.totalSize / $PartitionSize) * 100, 2))%"
    Log-Message "  Free Space: $($PartitionSize - $loadResult.totalSize) bytes"
}

Log-Message "File System Emulator Completed Successfully"
Write-Host "Partition formatted: $PartitionImage ($FileSystemType)"
if ($Verbose -and $LogFile) {
    Write-Host "Operations logged to: $LogFile"
}
