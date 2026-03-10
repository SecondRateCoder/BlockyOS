param(
    [Parameter(Mandatory=$false)]
    [string]$File = "./FAT32.iso",      # FAT32 image
    [Parameter(Mandatory=$true)]
    [string]$Target,    # Path inside FAT32
    [Parameter(Mandatory=$false)]
    [string]$Arg,       # Extra argument
    [Parameter(Mandatory=$true)]
    [string]$Cmd        # Command
)

# ------------------------------
# INTERNAL: Load or create image
# ------------------------------
function Load-Fat32 {
    if(Test-Path $File){
        return(Get-Content $File -Raw | ConvertFrom-Json)
    }else{
        return [ordered]@{
            System = @{
                MaxReadBytes = 4096
                MaxWriteBytes = 4096
            }
            Root = @{}
        }
    }
}

function Save-Fat32($img) {
    $img | ConvertTo-Json -Depth 20 | Set-Content $File
}

# ------------------------------
# INTERNAL: Resolve FAT32 path
# ------------------------------
function Resolve-Node($img, $path) {
    $parts = $path -split "/"
    $node = $img.Root
    foreach ($p in $parts) {
        if ($p -eq "") { continue }
        if (-not $node.ContainsKey($p)) { return $null }
        $node = $node[$p]
    }
    return $node
}

# ------------------------------
# ADD: Create file or directory
# ------------------------------
function Cmd-Add($img, $target, $arg) {
    $isDir = $arg.StartsWith("D")
    $name  = $arg.Substring(1)

    if (-not $isDir -and $arg.StartsWith("F")) {
        $name = $arg.Substring(1)
    } elseif (-not $arg.StartsWith("D") -and -not $arg.StartsWith("F")) {
        $name = $arg
    }

    $parent = Resolve-Node $img $target
    if ($null -eq $parent) { Write-Host "Invalid target"; return }

    if ($isDir) {
        $parent[$name] = @{}
    } else {
        $parent[$name] = @{
            Type = "File"
            Data = ""
        }
    }
}

# ------------------------------
# LIST: Show directory contents
# ------------------------------
function Cmd-List($img, $target) {
    $node = Resolve-Node $img $target
    if ($null -eq $node) { Write-Host "Invalid target"; return }

    foreach ($k in $node.Keys) {
        if ($node[$k] -is [hashtable]) {
            if ($node[$k].ContainsKey("Type")) {
                Write-Host "FILE  $k"
            } else {
                Write-Host "DIR   $k"
            }
        }
    }
}

# ------------------------------
# REMOVE: Delete file or directory
# ------------------------------
function Cmd-Remove($img, $target, $arg) {
    $parentPath = Split-Path $target
    $name = Split-Path $target -Leaf

    $parent = Resolve-Node $img $parentPath
    if ($null -eq $parent) { Write-Host "Invalid target"; return }

    $parent.Remove($name)
}

# ------------------------------
# POPULATE: Write real file into FAT
# ------------------------------
function Cmd-Populate($img, $target, $arg) {
    $node = Resolve-Node $img $target
    if ($null -eq $node -or $node.Type -ne "File") {
        Write-Host "Target is not a file"
        return
    }

    $bytes = [System.IO.File]::ReadAllBytes($arg)
    $node.Data = [System.Convert]::ToBase64String($bytes)
}

# ------------------------------
# READ: Extract file from FAT
# ------------------------------
function Cmd-Read($img, $target, $arg) {
    $node = Resolve-Node $img $target
    if ($null -eq $node -or $node.Type -ne "File") {
        Write-Host "Target is not a file"
        return
    }

    $bytes = [System.Convert]::FromBase64String($node.Data)

    if ([string]::IsNullOrWhiteSpace($arg)) {
        # Output to console as text
        Write-Host ([System.Text.Encoding]::UTF8.GetString($bytes))
    } else {
        [System.IO.File]::WriteAllBytes($arg, $bytes)
    }
}

# ------------------------------
# RELOCATE: Move or rename
# ------------------------------
function Cmd-Relocate($img, $target, $arg) {
    $srcParent = Resolve-Node $img (Split-Path $target)
    $srcName   = Split-Path $target -Leaf

    $dstParent = Resolve-Node $img (Split-Path $arg)
    $dstName   = Split-Path $arg -Leaf

    if ($null -eq $srcParent -or $null -eq $dstParent) {
        Write-Host "Invalid path"
        return
    }

    $dstParent[$dstName] = $srcParent[$srcName]
    $srcParent.Remove($srcName)
}

# ------------------------------
# SYSTEM: Read/Write settings
# ------------------------------
function Cmd-System($img, $target, $arg) {
    if ($target.StartsWith("R-")) {
        $key = $target.Substring(2)
        Write-Host $img.System[$key]
    }
    elseif ($target.StartsWith("W-")) {
        $key = $target.Substring(2)
        $img.System[$key] = $arg
    }
}

# ------------------------------
# MAIN EXECUTION
# ------------------------------
$img = Load-Fat32

switch ($Cmd.ToLower()) {
    "add"     { Cmd-Add      $img $Target $Arg }
    "create"  { Cmd-Add      $img $Target $Arg }
    "list"    { Cmd-List     $img $Target }
    "remove"  { Cmd-Remove   $img $Target $Arg }
    "rem"     { Cmd-Remove   $img $Target $Arg }
    "populate"{ Cmd-Populate $img $Target $Arg }
    "ins"     { Cmd-Populate $img $Target $Arg }
    "relocate"{ Cmd-Relocate $img $Target $Arg }
    "reloc"   { Cmd-Relocate $img $Target $Arg }
    "read"    { Cmd-Read     $img $Target $Arg }
    "system"  { Cmd-System   $img $Target $Arg }
    "reset"   {
        Remove-Item -Path $img -Force
        New-Item -Path $img -Force -ItemType File
    }
    default   { Write-Host "Unknown command" }
}

Save-Fat32 $img