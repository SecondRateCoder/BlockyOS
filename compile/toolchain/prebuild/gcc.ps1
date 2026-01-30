param(
    [Parameter(Mandatory=$true)]
    [string[]]$Params,
    [Parameter(Mandatory=$false)]
    [string]$Image = "gcc"
)
if ($Image -notin @("ld", "gcc", "as", "ar", "objcopy", "objdump")) {throw "Unsupported tool: $Image"}

$ROOT = @(
    (Join-Path (Get-Location) "compile/toolchain/prebuild/i686-elf/bin"),
    (Join-Path (Get-Location) "compile/toolchain/prebuild/i686-elf/i686-elf/bin")
)

$env:PATH = "$($ROOT -join ';');$($env:PATH)"
# Write-Host -Message "`n$($env:PATH -replace ';',"`n")" -ForegroundColor Yellow

$TrueParams = @()
$Params | ForEach-Object{
    $_ | ForEach-Object{
        ($_ -split ' ') | ForEach-Object{
            $TrueParams += $_
        }
    }
}

Write-Host ("i686-elf-$($Image).exe $($TrueParams -join ' ')") -ForegroundColor Yellow
& "i686-elf-$($Image).exe" "-fdiagnostics-color=always" @TrueParams 2>&1