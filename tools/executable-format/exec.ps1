param(
    [string[]]$CARGS = @(),
    [string]$OUTEXEC = (Join-Path $PSScriptRoot 'exechandler.exe')
)

$ToolRoot = $PSScriptRoot
$RepoRoot = (Get-Item $ToolRoot).Parent.Parent.FullName

if(Test-Path $OUTEXEC){Remove-Item $OUTEXEC -Force -ErrorAction Stop}

$GCC = 'gcc'
$CompilerArgs = @(
    '-I', $ToolRoot,
    '-I', $RepoRoot, '-O0',
    '-g', '-std=c99', '-fdiagnostics-color=always'
)
(Get-ChildItem -Path $ToolRoot, (Join-Path $RepoRoot 'tools\json') -Recurse -Include '*.c' -File) | ForEach-Object{$CompilerArgs += $_.FullName}
$CompilerArgs += $CARGS
$CompilerArgs += @('-o', $OUTEXEC)

Write-Host "`n$($GCC) $($CompilerArgs -join ' ')`n"
$GCCOUT = (& $GCC @CompilerArgs) 2>&1
Write-Host ($GCCOUT -join "`n")

exit $LASTEXITCODE