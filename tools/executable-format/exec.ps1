param(
    [string[]]$CARGS,
    [string]$OUTEXEC = (Join-Path (Get-Location) '\tools\executable-format\exechandler.exe')
)

$GCC = 'gcc'
$CARGS = @(
    '-I', "$(Join-Path (Get-Location) 'tools\executable-format')", '-g'
)

(Get-ChildItem -Path (Join-Path (Get-Location) 'tools\executable-format') -Recurse -Include @("*.c") -File) | Where-Object { $_.FullName -notmatch '\\test\\' -and $_.FullName -notmatch '/test/' } | ForEach-Object{
    # $o = "$(Join-Path (Get-Location) 'tools\executable-format')\$($_.BaseName).o"
    $CARGS += $_.FullName
}

$GCCOUT = (& $GCC $CARGS '-o' $OUTEXEC) 2>&1
Write-Host ($GCCOUT -join "`n")

exit 0