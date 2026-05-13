

$CFILES = @()
if(Test-Path ("$(Get-Location)/tools/filesystem/fs.exe")){Remove-Item "$(Get-Location)/tools/filesystem/fs.exe" -Force}
(Get-ChildItem -Path "$(Get-Location)/tools/filesystem/" -Recurse -Include "*.c" -File) | ForEach-Object {if($_.FullName -notmatch 'fat'){$CFILES += $_.FullName}}
$GCCOUT = & 'gcc' '-fdiagnostics-color=always' '-g' $CFILES '-o' "$(Get-Location)/tools/filesystem/fs.exe" 2>&1
Write-Host "$($GCCOUT -join "`n")"