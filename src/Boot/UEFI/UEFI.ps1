param(
	[Parameter(Mandatory=$true)]
	[string]$PREFIX,
	[Parameter(Mandatory=$true)]
	[string]$NASM,
	[Parameter(Mandatory=$true)]
	[string]$GCC,
	[Parameter(Mandatory=$true)]
	[string]$EMUOUT,
	[string]$ARCHITECTURE = 'x86_64',
	[switch]$ENABLEDEBUGGABLE,
	[switch]$ENABLENTEMULATOR
)

# $LD = $GCC -replace 'gcc\.exe', 'ld.exe'
# $OBJCOPY = $GCC -replace 'gcc\.exe', 'objcopy.exe'
$GCC = Join-Path (Get-Location) 'src/Boot/UEFI/gcc.ps1'

$EFIPARENT = Join-Path (Get-Location) "compile\toolchain\build\gnu-efi-4.0.4\$($ARCHITECTURE)/"
$BUILDDIR = Join-Path (Get-Location) "Build\Build-$($PREFIX)\"
$OBJDIR = Join-Path $BUILDDIR 'objs/'
$MAPFILE = Join-Path $BUILDDIR 'uefi-map.log'
$LOGFILE = Join-Path $BUILDDIR 'uefi.log'
$OFILES = @()
$UEFIBINARYBLOB = (Join-Path -Path $Objdir "/blob.bin")
$EMUUEFIBINARYBLOB = (Join-Path -Path $EMUOUT "/blob.efi")

$TEMPJSON = Join-Path (Get-Location) 'compile/cache/desc.json'
if(-not (Test-Path $TEMPJSON)){New-Item $TEMPJSON -ItemType File -Force}

$CARGS = @(
	'-I', "$(Get-Location)/src/", 
	'-I', "$($EFIPARENT)\include\efi\", '-I', "$($EFIPARENT)include\efi\$(if($ARCHITECTURE -eq 'x86_64'){'x86_64'}else{'ia32'})\", 
	 '-I',"$($EFIPARENT)include\efi\legacy\", '-I', "$($EFIPARENT)include\efi\protocol\",
	'-nostdlib', '-fdiagnostics-color=always', '-fshort-wchar', 
	'-fno-strict-aliasing', '-fno-stack-protector', '-fno-stack-check',
	'-ffreestanding', '-Wno-builtin-declaration-mismatch'
	"-m$(if($ARCHITECTURE -eq 'x86_64'){'64'}else{'32'})",
	'-D', "$(if($ARCHITECTURE -eq 'x86_64'){'__x86_64__'}else{'__ia32__', '-D', 'EFI32'})"
)
if($ENABLEDEBUGGABLE){$CARGS += '-D', 'EFI_DEBUG'}
if($ENABLENTEMULATOR){$CARGS += '-D', 'EFI_NT_EMULATOR'}
if($ARCHITECTURE -eq 'x86_64'){$CARGS += '-D', '__x86_64__', '-D', 'HAVE_USE_MS_ABI', '-mno-red-zone', '-maccumulate-outgoing-args'}
elseif($ARCHITECTURE -eq 'x86'){$CARGS += '-D', 'EFI32', '-D', '__ia32__'}


$LARGSU = @(
	'-nostdlib', "--sysroot=$($EFIPARENT)", '-Bsymbolic',
	"-Wl,-Map,$($MAPFILE)", "-Wl,-T,$($EFIPARENT)/lib/elf_$($ARCHITECTURE)_efi.lds"
	"$($EFIPARENT)/lib/crt0-efi-$($ARCHITECTURE).o",
	"$($EFIPARENT)/lib/reloc_$($ARCHITECTURE).o"
)
$LARGSL = @('-L', "$($EFIPARENT)/lib/", '-l', 'efi', '-l', 'gnuefi')

function Get-TimestampCache {
	param(
		[Parameter(Mandatory)]
		[string]$JsonPath
	)

	if(-not (Test-Path $JsonPath)){
		New-Item -Path $JsonPath -ItemType File -Force | Out-Null
		Set-Content -Path $JsonPath -Value '{}' -NoNewline
	}

	$raw = Get-Content -Path $JsonPath -Raw
	if([string]::IsNullOrWhiteSpace($raw)){
		$raw = '{}'
		Set-Content -Path $JsonPath -Value $raw -NoNewline
	}

	try{return $raw | ConvertFrom-Json -AsHashTable
	}catch{return @{}}
}

function Save-TimestampCache{
	param(
		[Parameter(Mandatory)]
		[string]$JsonPath,
		[Parameter(Mandatory)]
		$Data
	)
	$Data | ConvertTo-Json -Depth 4 | Set-Content -Path $JsonPath
}

function Test-JsonTimestampIsOutdated{
	param(
		[Parameter(Mandatory)]
		[string]$JsonPath,
		[Parameter(Mandatory)]
		[string]$FilePath
	)

	if(-not (Test-Path $FilePath)) { throw "Target file not found: $FilePath" }

	$json = Get-TimestampCache -JsonPath $JsonPath
	$fullPath = (Get-Item $FilePath).FullName

	$storedTimestamp = if ($json.ContainsKey($fullPath)) { $json[$fullPath] } else { $null }
	if(-not $storedTimestamp){return $true}

	try{$storedTime = [DateTime]::Parse($storedTimestamp)
	}catch{return $true}

	$actualTime = (Get-Item $FilePath).LastWriteTimeUtc
	if($actualTime -gt $storedTime.ToUniversalTime()){return $true}

	return $false
}

function Get-JsonTimestampInfo {
	param(
		[Parameter(Mandatory)]
		[string]$JsonPath,
		[Parameter(Mandatory)]
		[string]$FilePath
	)

	if(-not (Test-Path $FilePath)) { throw "Target file not found: $FilePath" }

	$json = Get-TimestampCache -JsonPath $JsonPath
	$fullPath = (Get-Item $FilePath).FullName
	$storedTimestamp = if ($json.ContainsKey($fullPath)) { $json[$fullPath] } else { $null }
	$storedTime = $null
	if($storedTimestamp){
		try{ $storedTime = [DateTime]::Parse($storedTimestamp).ToUniversalTime() } catch { $storedTime = $null }
	}
	$actualTime = (Get-Item $FilePath).LastWriteTimeUtc

	return [PSCustomObject]@{
		FilePath = $fullPath
		StoredTimestamp = $storedTimestamp
		StoredTime = $storedTime
		ActualTime = $actualTime
		Delta = if($storedTime){ $actualTime - $storedTime } else { $null }
	}
}

function Format-TimestampDiff {
	param(
		[Nullable[TimeSpan]]$Delta
	)

	if(-not $Delta){ return 'n/a' }
	return "{0:+0.000;-0.000;0.000}" -f $Delta.TotalSeconds
}

function Update-JsonTimestamp{
	param(
		[Parameter(Mandatory)]
		[string]$JsonPath,
		[Parameter(Mandatory)]
		[string]$FilePath
	)
	if(-not (Test-Path $FilePath)) { throw "Target file not found: $FilePath" }

	$json = Get-TimestampCache -JsonPath $JsonPath
	$fullPath = (Get-Item $FilePath).FullName
	$json[$fullPath] = (Get-Item $FilePath).LastWriteTimeUtc.ToString('o')
	Save-TimestampCache -JsonPath $JsonPath -Data $json
}

function Log-Write{
	param(
		[string]$Msg,
		[System.ConsoleColor]$color
	)
	$clean = ""
	$esc=[char]27
	if($color){
		Write-Host $Msg -ForegroundColor $color
		$clean = $Msg -replace "$esc(?:\[[0-9;?]*[ -/]*[@-~]|][^\a]*\a|P.*?$esc\\|X.*?$esc\\|\^.*?$esc\\|_.*?$esc\\|[@-Z\\-_])",""
	}else{
		Write-Host $Msg
		$clean = $Msg
	}
	if(-not (Test-Path $LOGFILE)){
		New-Item $BUILDDIR -ItemType Directory -ErrorAction SilentlyContinue
		New-Item $LOGFILE -ItemType File
	}
	$success = $false
	do{
		$success = $true
		try{
			if(-not (Test-Path $LOGFILE)){New-Item $LOGFILE -ItemType File}
			Add-Content -Path $LOGFILE -Value $clean
		}catch{$success = $true}
	}while($success -eq $false)
}

function Ensure-PosixUefiRuntime {
	$POSIXUEFI_LIB = "$($EFIPARENT)/lib"
	if(-not (Test-Path $POSIXUEFI_LIB)){
		Log-Write "GNU-EFI library does not exist: $POSIXUEFI_LIB"
		throw ''
	}
}

(Ensure-PosixUefiRuntime)

# Compile all UEFI .c files
Get-ChildItem -Path @((Join-Path (Get-Location) "src/Boot/UEFI/")) -Include "*.c" -Recurse -File | ForEach-Object{
	$f = $_
	$o = Join-Path $Objdir "UEFI.c.$($_.BaseName).o"
	$tsInfo = Get-JsonTimestampInfo -JsonPath $TEMPJSON -FilePath $f.FullName
	$needsCompile = (-not $tsInfo.StoredTimestamp) -or (-not $tsInfo.StoredTime) -or ($tsInfo.ActualTime -gt $tsInfo.StoredTime)
	if($needsCompile){
		Log-Write -Msg "Stale source detected for $($f.FullName). source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Yellow
		Log-Write -Msg "$($GCC) -c $($_.FullName) -o $($o) $($CARGS -join ' ')" -color Blue
		# $GCCOUT = & $GCC '-c' "$($_.FullName)" '-o' $o $CARGS 2>&1
		$GCCOUT = & $GCC -_ARGS @('-c', "$($_.FullName)", '-o', $o, $CARGS)
		Log-Write -Msg ($GCCOUT -join "`n")
		if(-not (Test-Path $o)){exit 1}
		Log-Write -Msg "Compiled and updated timestamp for $($f.FullName)" -color Green
		Update-JsonTimestamp -JsonPath $TEMPJSON -FilePath $f.FullName
	}else{Log-Write -Msg "Skipping $($f.FullName) - up to date; source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp)" -color Green}
	$OFILES += $o
}

# $LDOUT = & $LD $LARGS $OFILES '-o' $UEFIBINARYBLOB 2>&1
Log-Write "$($GCC) $($LARGSU -join ' ') $($OFILES -join ' ') $($LARGSL -join ' ')-o $($UEFIBINARYBLOB)" -color Blue
$LDOUT = & $GCC -_ARGS @($LARGSU, $OFILES, $LARGSL, '-o', $UEFIBINARYBLOB)
Log-Write "$($LDOUT -join "`n")"
if(Test-Path $UEFIBINARYBLOB){Copy-Item $UEFIBINARYBLOB $EMUUEFIBINARYBLOB}

# Update timestamps for header files
Get-ChildItem -Path @((Join-Path (Get-Location) "src/Boot/UEFI"), "$($EFIPARENT)/") -Include @("*.h", "*.c", "*.asm") -Recurse -File | ForEach-Object{
	$f = $_
	if(Test-JsonTimestampIsOutdated -JsonPath $TEMPJSON -FilePath $f.FullName){
		$tsInfo = Get-JsonTimestampInfo -JsonPath $TEMPJSON -FilePath $f.FullName
		Update-JsonTimestamp -JsonPath $TEMPJSON -FilePath $f.FullName
		Log-Write -Msg "Updated timestamp for header file: $($f.FullName); source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Yellow
	}
}