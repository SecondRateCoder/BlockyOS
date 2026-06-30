param(
    [Parameter(Mandatory=$true)]
    [string[]]$_ARGS,
    [Parameter(Mandatory=$false)]
    [string]$IMAGE = 'gcc'
)
$envOLDPATH = $env:PATH
$TYPE = $(if($_ARGS -contains "-m32"){'i686-elf'}else{'x86_64-elf'})
$ROOT = @(
    (Join-Path (Get-Location) "compile/toolchain/prebuild/$($TYPE)/bin/"),
    (Join-Path (Get-Location) "compile/toolchain/prebuild/$($TYPE)/$($TYPE)/bin/")
)
$env:PATH = "$($ROOT -join ';');$($env:PATH)"
$env:COMPILER_PATH = $ROOT[0]

$ARGS_ = @()
$_ARGS | ForEach-Object{$_ | ForEach-Object{($_ -split ' ') | ForEach-Object{$ARGS_ += $_}}}

$GCCOUT = $null
if($_ARGS -contains "-m32"){$GCCOUT = (& "$($TYPE)-$($IMAGE)" $ARGS_) 2>&1
}else{$GCCOUT = (& "$($TYPE)-$($IMAGE)" $ARGS_) 2>&1}

$env:COMPILER_PATH = $null
$env:PATH = $envOLDPATH

return $GCCOUT