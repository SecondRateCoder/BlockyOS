param(
    [Parameter(Mandatory=$false)]
    [string]$IMAGE = 'gcc',
    [Parameter(Mandatory=$true)]
    [string[]]$ARGSS
)

$ROOT = @(
    (Join-Path (Get-Location) "compile\toolchain\prebuild\$(if($ARGSS -contains "-m32"){'i686-elf'}else{'x86_64-elf'})\bin"),
    (Join-Path (Get-Location) "compile\toolchain\prebuild\$(if($ARGSS -contains "-m32"){'i686-elf'}else{'x86_64-elf'})\$(if($ARGSS -contains "-m32"){'i686-elf'}else{'x86_64-elf'})\bin")
)
$envDupe = $env:Path
$envCompPathDupe = $env:COMPILER_PATH
$env:PATH = "$($ROOT -join ';');$($env:PATH)"
$env:COMPILER_PATH = $ROOT[0]
$out = $null

$ARGS_ = @()
$ARGSS | ForEach-Object{$_ | ForEach-Object{($_ -split ' ') | ForEach-Object{$ARGS_ += $_}}}
if($ARGS_ -contains "-m32"){
    Write-Host "i686-elf-$($IMAGE) $($ARGS_ -join ' ')" -ForegroundColor Yellow
    $out = (& "i686-elf-$($IMAGE)" $ARGS_) 2>&1
}else{
    Write-Host "x86_64-elf-$($IMAGE) $($ARGS_ -join ' ')" -ForegroundColor Yellow
    $out = (& "x86_64-elf-$($IMAGE)" $ARGS_) 2>&1
}

$env:PATH = $envDupe
$env:COMPILER_PATH = $envCompPathDupe

return $out