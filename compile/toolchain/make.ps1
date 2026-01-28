<#
install-i686-elf-toolchain.ps1
PowerShell installer for a prebuilt i686-elf cross toolchain.

Usage:
  .\install-i686-elf-toolchain.ps1 -Url "https://example.com/i686-elf-toolchain.zip" -InstallDir "C:\cross"

Notes:
  - Provide a direct download URL to a zip or tar.xz archive containing a bin/ directory with i686-elf-* tools.
  - If the archive is tar.xz, 7z must be installed and on PATH (7z.exe).
#>

param(
	[Parameter(Mandatory=$true)]
	[string]$Url = 
		"https://release-assets.githubusercontent.com/github-production-release-asset/47872348/98f109e4-fd49-482d-834d-39c52c73cfe8?sp=r&sv=2018-11-09&sr=b&spr=https&se=2026-01-27T19%3A28%3A27Z&rscd=attachment%3B+filename%3Di686-elf-tools-windows.zip&rsct=application%2Foctet-stream&skoid=96c2d410-5711-43a1-aedd-ab1947aa7ab0&sktid=398a6654-997b-47e9-b12b-9515b896b4de&skt=2026-01-27T18%3A27%3A37Z&ske=2026-01-27T19%3A28%3A27Z&sks=b&skv=2018-11-09&sig=HJ1FOyhu0z8ZLY3qMDKtBH8QO182sC0J3kV4tgA04Ns%3D&jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmVsZWFzZS1hc3NldHMuZ2l0aHVidXNlcmNvbnRlbnQuY29tIiwia2V5Ijoia2V5MSIsImV4cCI6MTc2OTU0MzQ5NywibmJmIjoxNzY5NTM5ODk3LCJwYXRoIjoicmVsZWFzZWFzc2V0cHJvZHVjdGlvbi5ibG9iLmNvcmUud2luZG93cy5uZXQifQ.UPJQoxArKGDNN1-tEfMeXeWl0e1XVDxlUaxpJaFcF7w&response-content-disposition=attachment%3B%20filename%3Di686-elf-tools-windows.zip&response-content-type=application%2Foctet-stream",
	[string]$InstallDir = "C:\msys64\elf-mysys32",
	[switch]$Force
)

function Write-Err([string]$m){Write-Host $m -ForegroundColor Red}
function Write-Ok([string]$m){Write-Host $m -ForegroundColor Green}

# Prepare
$Temp = Join-Path $env:TEMP ("i686-elf-toolchain-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $Temp | Out-Null

$archive = Join-Path $Temp (Split-Path $Url -Leaf)
Write-Host "Downloading toolchain from $Url ..."
try{Invoke-WebRequest -Uri $Url -OutFile $archive -UseBasicParsing -ErrorAction Stop
}catch{
	Write-Err "Download failed: $($_.Exception.Message)"
	exit 1
}

# Create install dir
if(Test-Path $InstallDir){
  	if(-not $Force){
		Write-Host "Install directory $InstallDir already exists. Use -Force to overwrite."
		Remove-Item $Temp -Recurse -Force
		exit 1
  	}else{
		Write-Host "Removing existing $InstallDir (force)..."
		Remove-Item $InstallDir -Recurse -Force
  	}
}
New-Item -ItemType Directory -Path $InstallDir | Out-Null

# Extract archive
$ext = [IO.Path]::GetExtension($archive).ToLowerInvariant()
if($ext -eq ".zip"){
	Write-Host "Extracting zip..."
	Expand-Archive -LiteralPath $archive -DestinationPath $Temp -Force
}elseif($archive -match "\.tar\.xz$" -or $ext -eq ".xz"){
	# Need 7z
	$seven = (Get-Command 7z.exe -ErrorAction SilentlyContinue)
	if(-not $seven){
		Write-Err "7z.exe not found. Install 7-Zip and ensure 7z.exe is on PATH."
		Remove-Item $Temp -Recurse -Force
		exit 1
	}
	Write-Host "Extracting tar.xz with 7z..."
	& 7z.exe x -y $archive -o$Temp | Out-Null
	# 7z will produce a .tar; extract it
	$tar = Get-ChildItem -Path $Temp -Filter *.tar -Recurse | Select-Object -First 1
	if(-not $tar){
		Write-Err "No .tar found after extracting .xz"
		Remove-Item $Temp -Recurse -Force;
		exit 1
	}
	& 7z.exe x -y $tar.FullName -o$Temp | Out-Null
}else{
	Write-Err "Unsupported archive extension: $ext. Provide .zip or .tar.xz"
	Remove-Item $Temp -Recurse -Force
  	exit 1
}

# Find top-level folder that contains bin\i686-elf-gcc or bin/i686-elf-gcc
$binCandidate = Get-ChildItem -Path $Temp -Recurse -Filter "i686-elf-gcc.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if(-not $binCandidate){
  # maybe unix-named binaries without .exe
  $binCandidate = Get-ChildItem -Path $Temp -Recurse -Filter "i686-elf-gcc" -ErrorAction SilentlyContinue | Select-Object -First 1
}
if(-not $binCandidate){
  Write-Err "Could not find i686-elf-gcc in the extracted archive. Ensure the archive contains a bin/ with i686-elf-* tools."
  Remove-Item $Temp -Recurse -Force
  exit 1
}

# Determine root of extracted tree (parent of bin)
$binDir = Split-Path $binCandidate.FullName -Parent
$rootDir = Split-Path $binDir -Parent

Write-Host "Copying toolchain files to $InstallDir ..."
# Copy contents of rootDir into InstallDir
Copy-Item -Path (Join-Path $rootDir '*') -Destination $InstallDir -Recurse -Force

# Add to current session PATH
$binPath = Join-Path $InstallDir 'bin'
if(-not(Test-Path $binPath)){
  Write-Err "Expected bin directory not found at $binPath"
  Remove-Item $Temp -Recurse -Force
  exit 1
}
$env:PATH = "$binPath;$env:PATH"
Write-Ok "Toolchain installed to $InstallDir"
Write-Host "Temporary files in $Temp (will be removed)."

# Verify
Write-Host "Verifying i686-elf-gcc..."
try{
  $ver = & (Join-Path $binPath 'i686-elf-gcc') --version 2>&1
  Write-Host $ver
}catch{
  Write-Err "Failed to run i686-elf-gcc. If the binary is a Linux ELF (no .exe), it won't run on Windows. You need a Windows-native build."
  Remove-Item $Temp -Recurse -Force
  exit 1
}

# Cleanup
Remove-Item $Temp -Recurse -Force
Write-Ok "Done. To persist the toolchain in future sessions, add $binPath to your system/user PATH."