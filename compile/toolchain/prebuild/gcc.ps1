param(
    [Parameter(Mandatory=$true)]
    [string[]]$Params,
    [Parameter(Mandatory=$false)]
    [string]$Image = "gcc"
)
if ($Image -notin @("ld", "gcc", "as", "ar", "objcopy", "objdump")){throw "Unsupported tool: $($Image)"}
if($Image -ne 'gcc'){$Image = (Join-Path (Get-Location) "compile/toolchain/prebuild/binutils/bin/$($Image).exe")}

$ROOT = @(
    (Join-Path (Get-Location) "compile/toolchain/prebuild/i686-elf/bin"),
    (Join-Path (Get-Location) "compile/toolchain/prebuild/i686-elf/i686-elf/bin")
)
$env:PATH = "$($ROOT -join ';');$($env:PATH)"
$env:COMPILER_PATH = $ROOT[0]

$TrueParams = @('-B', (Join-Path (Get-Location) "compile/toolchain/prebuild/binutils/bin/"), '-fno-lto', '--sysroot=', '-masm=att')
$Params | ForEach-Object{
    $_ | ForEach-Object{
        ($_ -split ' ') | ForEach-Object{
            $TrueParams += $_
        }
    }
}

Write-Host ("i686-elf-$($Image).exe $($TrueParams -join ' ')") -ForegroundColor Blue

return (& "i686-elf-$($Image).exe" @TrueParams 2>&1)