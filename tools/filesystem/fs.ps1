param(
	[Parameter(Mandatory=$false)]
	[bool]$RELEASE = $false
)

$ToolRoot = $PSScriptRoot
$OutputPath = Join-Path $ToolRoot 'fs.exe'
$CFILES = @()
if(Test-Path $OutputPath){Remove-Item $OutputPath -Force -ErrorAction Stop}
(Get-ChildItem -Path $ToolRoot -Recurse -Include '*.c' -File) | ForEach-Object {if($_.FullName -notmatch 'fat'){$CFILES += $_.FullName}}
$GCCOUT = ""
if($RELEASE){$GCCOUT = & 'gcc' '-fdiagnostics-color=always' '-O2' @CFILES '-o' $OutputPath $(if($IsWindows){'-lbcrypt'}) 2>&1
}else{$GCCOUT = & 'gcc' '-fdiagnostics-color=always' '-g' '-D' '_DEBUG' @CFILES '-o' $OutputPath $(if($IsWindows){'-lbcrypt'}) 2>&1}
Write-Host "$($GCCOUT -join "`n")"
exit $LASTEXITCODE