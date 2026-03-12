param(
    [Parameter(Mandatory=$true)]
    [string]$prefix,
    [Parameter(Mandatory=$true)]
    [string]$NASM,
    [Parameter(Mandatory=$true)]
    [string]$GCC,
    <#
        This contains an Array of Directories from a Base e.g [Base/][F/D]/File/File...
        Where [Base/] is excluded and the Path includes [F] or [D]. if [F] then the Format is PATH:TRUE PATH
    #>
    [Parameter(Mandatory=$true)]
    [string[]]$ProxyFileSystem
)

$Build = Join-Path (Get-Location) ("Build\Build-" + $prefix)
$Log = Join-Path $Build ("logcompile.txt")
$Objdir = Join-Path $Build "\objs\"
$PROXYFS = Join-Path $Objdir "\proxy\file-system\"
if(-not (Test-Path $PROXYFS)){New-Item $PROXYFS -ItemType Directory}
$SRC = Join-Path (Get-Location) '/src/Boot/'
$FAT32CONFIG = "$($SRC)/FAT32/configure.ps1"
$FAT32EMU = "$($SRC)/FAT32/FAT32.ps1"

$LST1BIN = Join-Path -Path $Objdir "boot1.bin"
$LST2BIN = Join-Path -Path $Objdir "boot2.bin"

function Log-Write{
    param(
        [string]$Msg,
        [System.ConsoleColor]$color
    )
    $clean = ""
    $esc=[char]27
    if($color){
        Write-Host $Msg -ForegroundColor $color
        $clean = $Msg -replace "$esc(?:\[[0-9;?]*[ -/]*[@-~]|][^\a]*\a|P.*?$esc\\|X.*?$esc\\|\^.*?$esc\\|_.*?$esc\\|[@-Z\\-_])",""
    }else{
        Write-Host $Msg
        $clean = $Msg
    }
    $success = $false
    do{
        $success = $true
        try{
            if(-not (Test-Path $Log)){New-Item $Log -ItemType File}
            Add-Content -Path $Log -Value $clean
        }catch{$success = $true}
    }while($success -eq $false)
}

function Generate-PFS{
    param([string[]]$PFSDesc)
    # Generate Proxy File-System
    if(Test-Path $PROXYFS){Remove-Item $PROXYFS -Force}
        New-Item $PROXYFS -ItemType Directory -Force
    foreach($path in $PFSDesc){
        if(($path[0] -eq '[') -and ($path[0] -eq 'D') -and ($path[0] -eq ']')){
            New-Item (Join-Path $PROXYFS $path) -ItemType Directory
        }elseif(($path[0] -eq '[') -and ($path[0] -eq 'F') -and ($path[0] -eq ']')){
            [string[]]$array = $path -split ':'
            if($array.Count -ne 2){
                Log-Write -color Red -Msg "Error! The appropiate Format was not found:`n[F]/File/File...:<TRUE PATH>"
                exit 1
            }
            [System.IO.FileInfo]$f = Get-Item $array[1]
            if($f){
                New-Item (Join-Path $PROXYFS $array[0]) -ItemType File
                Copy-Item -Path $f.FullName -Destination (Join-Path $PROXYFS $array[0])
            }else{
                Log-Write -color Red -Msg "Error! The attached Path was not found"
                exit 1
            }
        }
    }
}

(& "$($SRC)/Legacy/stage1/st1.ps1" -prefix $prefix -NASM $NASM)
(& "$($SRC)/Legacy/stage2/st2.ps1" -prefix $prefix -NASM $NASM -GCC $GCC)
(& "$($SRC)/UEFI/UEFI.ps1" -prefix $prefix -NASM $NASM -GCCPS1 $GCC)
if(-not (Test-Path (Join-Path -Path $Objdir "UEFI/table.json"))){
    New-Item (Join-Path -Path $Objdir "UEFI/") -ItemType Directory
    New-Item (Join-Path -Path $Objdir "UEFI/table.json") -ItemType File
}
[hashtable]$UEFIBINARIESTABLE = Get-Item (Join-Path -Path $Objdir "UEFI/table.json") | ConvertFrom-Json -AsHashTable


(Generate-PFS -PFSDesc $ProxyFileSystem)
$UEFIProxy = @()
foreach($kv in $UEFIBINARIESTABLE.GetEnumerator()){
    $UEFIProxy += "[F]/$($kv.Key):$($kv.Value)"
}
(Generate-PFS -PFSDesc $UEFIProxy)

(& $FAT32CONFIG -SourceDir $PROXYFS -TargetPath "/")

(& $FAT32EMU -Target "/Legacy/" -Arg "LegacyBlob.bin" -Cms "add")
New-Item -Path (Join-Path $Objdir "LegacyBlob.bin") -ItemType File -Force
Add-Content -Path (Join-Path $Objdir "LegacyBlob.bin") -Value (Get-Content $LST1BIN)
Add-Content -Path (Join-Path $Objdir "LegacyBlob.bin") -Value (Get-Content $LST2BIN)
(& $FAT32EMU -Target "/Legacy/LegacyBlob.bin" -Arg (Join-Path $Objdir "LegacyBlob.bin") -Cmd "ins")