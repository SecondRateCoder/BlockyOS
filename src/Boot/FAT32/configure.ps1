<#
.SYNOPSIS
    Import a real directory tree into a virtual FAT32 image using the Fat32Emu.ps1 CLI.

.DESCRIPTION
    The script calls the emulator script for each directory and file:
      - Creates directories via Add (Arg prefixed with 'D')
      - Creates file entries via Add (Arg prefixed with 'F') then Populate to copy real file bytes
    It enforces a conservative cap on the FAT blob size:
      sectors = ((1024 * 1024) / 512) + 3 = 2051
      capBytes = 2051 * 512 = 1,050,112

PARAMETERS
    -EmuScript   Path to your Fat32Emu.ps1 (the emulator CLI).
    -ImageFile   Path to the FAT image JSON file used by the emulator.
    -SourceDir   Real directory to import.
    -TargetPath  Target path inside the virtual FAT (e.g., "/" or "/MyFolder").
    -ReservedBytes Optional reserve for metadata overhead (default 65536).
    -DryRun      If set, the script will only simulate and print actions.
#>

param(
    [Parameter(Mandatory=$false)][string]$EmuScript = "./FAT32.ps1",
    [Parameter(Mandatory=$false)][string]$ImageFile = "./FAT32.iso",
    [Parameter(Mandatory=$true)][string]$SourceDir,
    [Parameter(Mandatory=$true)][string]$TargetPath,
    [int]$ReservedBytes = 65536,
    [switch]$DryRun
)

# -------------------------
# Constants and calculations
# -------------------------
$sectors = ((1024 * 1024) / 512) + 3
$capBytes = [int]($sectors * 512)            # 1,050,112 bytes
Write-Host "Sector cap:" $sectors "sectors"
Write-Host "Byte cap:" $capBytes "bytes"
Write-Host "Reserved bytes for metadata:" $ReservedBytes

# -------------------------
# Helpers
# -------------------------
function Normalize-FatPath {
    param([string]$p)
    if ([string]::IsNullOrWhiteSpace($p)) { return "/" }
    $pp = $p -replace "\\","/"
    if (-not $pp.StartsWith("/")) { $pp = "/" + $pp }
    return $pp.TrimEnd("/")
}

function Run-Emu {
    param(
        [string]$Cmd,
        [string]$Target,
        [string]$Arg
    )
    $argsList = @(
        "-File", $ImageFile,
        "-Target", $Target,
        "-Arg", $Arg,
        "-Cmd", $Cmd
    )
    if ($DryRun) {
        Write-Host "[DRYRUN] Emu call:" $EmuScript $argsList
        return @{ ExitCode = 0; StdOut = "[DRYRUN]" }
    }
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = "powershell.exe"
    $psi.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$EmuScript`" " + ($argsList -join " ")
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false
    $proc = [System.Diagnostics.Process]::Start($psi)
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()
    return @{ ExitCode = $proc.ExitCode; StdOut = $stdout; StdErr = $stderr }
}

function Estimate-EncodedSize {
    param([long]$rawBytes)
    # Base64 expands to 4/3 of raw bytes; round up to multiple of 4
    $b64 = [math]::Ceiling($rawBytes / 3) * 4
    # Add JSON/field overhead estimate per file
    $overhead = 200
    return [long]($b64 + $overhead)
}

# -------------------------
# Validate inputs
# -------------------------
(& $EmuScript -File "" -Target "" -Cmd "reset")
if (-not (Test-Path $EmuScript)) { throw "Emulator script not found: $EmuScript" }
if (-not (Test-Path $SourceDir)) { throw "Source directory not found: $SourceDir" }

$TargetPath = Normalize-FatPath $TargetPath
Write-Host "Mapping real directory '$SourceDir' -> FAT path '$TargetPath'"

# -------------------------
# Build list of directories and files
# -------------------------
$allDirs = Get-ChildItem -Path $SourceDir -Directory -Recurse -Force | ForEach-Object { $_.FullName }
# include root directory itself
$allDirs = ,$SourceDir + $allDirs
# Sort directories by path depth so parents are created before children
$allDirs = $allDirs | Sort-Object { ($_ -split "[\\/]").Count }

$allFiles = Get-ChildItem -Path $SourceDir -File -Recurse -Force

# -------------------------
# Track cumulative size
# -------------------------
$cumulativeEstimated = 0
$limit = $capBytes - $ReservedBytes
Write-Host "Effective data limit (cap - reserved):" $limit "bytes"

# -------------------------
# Create directories in FAT image
# -------------------------
foreach ($dir in $allDirs) {
    # compute relative path from source root
    $rel = $dir.Substring($SourceDir.Length).TrimStart("\","/")
    $fatDir = if ($rel -eq "") { $TargetPath } else { (Normalize-FatPath ($TargetPath + "/" + ($rel -replace "\\","/"))) }
    # create directory entry name and parent
    $parent = Split-Path $fatDir -Parent
    if ($parent -eq "") { $parent = "/" }
    $name = Split-Path $fatDir -Leaf
    if ($name -eq "") { continue } # root
    $arg = "D" + $name
    Write-Host "Creating directory in FAT:" $fatDir " (Parent:" $parent ")"
    $res = Run-Emu -Cmd "Add" -Target $parent -Arg $arg
    if ($res.ExitCode -ne 0) {
        Write-Host "Warning: emulator returned non-zero for Add directory:" $res.StdErr
    }
}

# -------------------------
# Add and populate files
# -------------------------
foreach ($f in $allFiles) {
    $rel = $f.FullName.Substring($SourceDir.Length).TrimStart("\","/")
    $fatFilePath = Normalize-FatPath ($TargetPath + "/" + ($rel -replace "\\","/"))
    $parent = Split-Path $fatFilePath -Parent
    if ($parent -eq "") { $parent = "/" }
    $name = Split-Path $fatFilePath -Leaf

    # Estimate encoded size
    $rawSize = (Get-Item $f.FullName).Length
    $est = Estimate-EncodedSize $rawSize

    if (($cumulativeEstimated + $est) -gt $limit) {
        Write-Host "Capacity reached. Skipping file:" $f.FullName
        Write-Host "Estimated next size:" $est "bytes; cumulative:" $cumulativeEstimated "limit:" $limit
        continue
    }

    # Create file entry
    $addArg = "F" + $name
    Write-Host "Adding file entry:" $fatFilePath "size:" $rawSize "estEncoded:" $est
    $resAdd = Run-Emu -Cmd "Add" -Target $parent -Arg $addArg
    if ($resAdd.ExitCode -ne 0) {
        Write-Host "Error adding file entry:" $resAdd.StdErr
        continue
    }

    # Populate file with real data
    $resPop = Run-Emu -Cmd "Populate" -Target $fatFilePath -Arg $f.FullName
    if ($resPop.ExitCode -ne 0) {
        Write-Host "Error populating file:" $resPop.StdErr
        # Optionally remove the file entry to keep image clean
        Run-Emu -Cmd "Remove" -Target $fatFilePath -Arg ""
        continue
    }

    $cumulativeEstimated += $est
    Write-Host "Cumulative estimated size now:" $cumulativeEstimated "bytes"
}

# -------------------------
# Final size check and summary
# -------------------------
if (-not $DryRun) {
    if (Test-Path $ImageFile) {
        $imgSize = (Get-Item $ImageFile).Length
        Write-Host "Final image file size:" $imgSize "bytes"
        if ($imgSize -gt $capBytes) {
            Write-Host "Warning: image file exceeds capBytes ($capBytes). Consider increasing ReservedBytes or reducing content."
        }
    } else {
        Write-Host "Image file not found after operations. Emulator may store image elsewhere or failed to save."
    }
}

Write-Host "Import complete. Files imported:" $allFiles.Count "Directories created:" $allDirs.Count