
$GCC = 'gcc'
$OUTFILE = (Join-Path (Get-Location) 'tools\executable-format\test\test.exe')
$FILES = @(
    (Join-Path (Get-Location) 'tools\executable-format\test\test.c')
)

& $GCC $FILES '-o' $OUTFILE

return $(if(Test-Path $OUTFILE){$OUTFILE}else{$null})