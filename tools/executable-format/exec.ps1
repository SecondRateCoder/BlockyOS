param(
    [string[]]$CARGS,
    [string]$OUTEXEC = (Join-Path (Get-Location) '\tools\executable-format\exechandler.exe')
)

if(Test-Path $OUTEXEC){Remove-Item $OUTEXEC -Force}

$GCC = 'gcc'
$CARGS = @(
    '-I', "$(Join-Path (Get-Location) 'tools\executable-format')", 
    '-I', "$(Get-Location)", '-O0', 
    '-g', '-std=c99', '-fdiagnostics-color=always'
)
(Get-ChildItem -Path @((Join-Path (Get-Location) 'tools\executable-format\'), (Join-Path (Get-Location) 'tools\json')) -Recurse -Include @("*.c") -File) | ForEach-Object{$CARGS += $_.FullName}

Write-Host "`n$($GCC) $($CARGS) -o $($OUTEXEC)`n"
$GCCOUT = (& $GCC $CARGS '-o' $OUTEXEC) 2>&1
Write-Host ($GCCOUT -join "`n")

exit 0