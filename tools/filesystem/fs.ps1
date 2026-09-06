param(
	[Parameter(Mandatory=$false)]
	[bool]$RELEASE = $false
)

$CFILES = @()
if(Test-Path ("$(Get-Location)/tools/filesystem/fs.exe")){Remove-Item "$(Get-Location)/tools/filesystem/fs.exe" -Force}
(Get-ChildItem -Path "$(Get-Location)/tools/filesystem/" -Recurse -Include "*.c" -File) | ForEach-Object {if($_.FullName -notmatch 'fat'){$CFILES += $_.FullName}}
$GCCOUT = ""
if($RELEASE){$GCCOUT = & 'gcc' '-fdiagnostics-color=always' '-O2' $CFILES '-o' "$(Get-Location)/tools/filesystem/fs.exe" $(if($IsWindows){'-lbcrypt'}) 2>&1
}else{$GCCOUT = & 'gcc' '-fdiagnostics-color=always' '-g' '-D' '_DEBUG' $CFILES '-o' "$(Get-Location)/tools/filesystem/fs.exe" $(if($IsWindows){'-lbcrypt'}) 2>&1}
Write-Host "$($GCCOUT -join "`n")"