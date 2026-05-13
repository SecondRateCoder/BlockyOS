param(
    [Parameter(Mandatory=$true)]
    [string[]]$_ARGS,
    [Parameter(Mandatory=$false)]
    [string]$IMAGE = 'gcc'
)

$ROOT = @(
    (Join-Path (Get-Location) "compile/toolchain/prebuild/$(if($_ARGS -contains "-m32"){'i686-elf'}else{'x86_64-elf'})/bin"),
    (Join-Path (Get-Location) "compile/toolchain/prebuild/$(if($_ARGS -contains "-m32"){'i686-elf'}else{'x86_64-elf'})/$(if($_ARGS -contains "-m32"){'i686-elf'}else{'x86_64-elf'})/bin")
)
$env:PATH = "$($ROOT -join ';');$($env:PATH)"
$env:COMPILER_PATH = $ROOT[0]

$ARGS_ = @()
$_ARGS | ForEach-Object{$_ | ForEach-Object{($_ -split ' ') | ForEach-Object{$ARGS_ += $_}}}

if($_ARGS -contains "-m32"){return (& "i686-elf-$($IMAGE)" $ARGS_) 2>&1
}else{return (& "x86_64-elf-$($IMAGE)" '-nostdlib' '-nostartfiles' '-nodefaultlibs' $ARGS_) 2>&1}