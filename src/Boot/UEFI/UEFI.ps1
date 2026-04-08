param(
	[Parameter(Mandatory=$true)]
	[string]$prefix,
	[Parameter(Mandatory=$true)]
	[string]$NASM,
	[Parameter(Mandatory=$true)]
	[string]$GCC,
	[Parameter(Mandatory=$true)]
	[string]$EMUOUT
)

# $LD = $GCC -replace 'gcc\.exe', 'ld.exe'
# $OBJCOPY = $GCC -replace 'gcc\.exe', 'objcopy.exe'

$EFIPARENT = Join-Path (Get-Location) "compile/toolchain/posix-uefi/"
$ARCHITECTURE = "x86"
$ARCHEXCLUDE = @('aarch64', 'riscv64')

$Build = Join-Path (Get-Location) ("Build\Build-" + $prefix)
if(-not (Test-Path $Build)){New-Item $Build -ItemType Directory}
$Log = Join-Path $Build ("/logUEFI.log")
$Objdir = Join-Path $Build "\objs\"
if(-not (Test-Path $Objdir)){New-Item $Objdir -ItemType Directory}
$MAPFILE = Join-Path $Build "UEFImap.log"
if(-not (Test-Path $MAPFILE)){New-Item $MAPFILE -ItemType File}

$TEMPJSON = Join-Path (Get-Location) 'compile/cache/desc.json'
if(-not (Test-Path $TEMPJSON)){New-Item $TEMPJSON -ItemType File -Force}

$Objects = @()

$UEFIBINARYBLOB = (Join-Path -Path $Objdir "/blob.efi")
$EMUUEFIBINARYBLOB = (Join-Path -Path $EMUOUT "/blob.efi")
$POSIXUEFI_BUILD = Join-Path $EFIPARENT "build/uefi"
$POSIXUEFI_CRT = Join-Path $EFIPARENT "uefi/crt_$($ARCHITECTURE).o"
$POSIXUEFI_LIB = Join-Path $POSIXUEFI_BUILD "libuefi.a"
$ASM_FORMAT = if($ARCHITECTURE -eq 'x86_64'){ 'elf64' } else { 'i386' }

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
	if(-not (Test-Path $Log)){
		New-Item $Build -ItemType Directory -ErrorAction SilentlyContinue
		New-Item $Log -ItemType File
	}
	$success = $false
    do{
        $success = $true
        try{
            if(-not (Test-Path $Log)){New-Item $Log -ItemType File}
            Add-Content -Path $Log -Value $clean
        }catch{$success = $true}
    }while($success -eq $false)
}

function Ensure-PosixUefiRuntime {
    if(-not (Test-Path $POSIXUEFI_LIB) -or -not (Test-Path $POSIXUEFI_CRT)){
        Log-Write -Msg "Building POSIX-UEFI runtime library..." -color Yellow
        Push-Location "$($EFIPARENT)/uefi"
        if(-not (Get-Command mingw32-make -ErrorAction SilentlyContinue)){
            Log-Write "Make is required to build the POSIX-UEFI runtime, but make was not found in PATH."
			throw ''
        }
        $makeOut = & mingw32-make "USE_GCC=1" "ARCH=$ARCHITECTURE" 2>&1
        Pop-Location
        if($LASTEXITCODE -ne 0){
            Log-Write -Msg ($makeOut -join "`n") -color Red
            Log-Write "Failed to build POSIX-UEFI runtime library."
			throw ''
        }
        if(-not (Test-Path $POSIXUEFI_LIB) -or -not (Test-Path $POSIXUEFI_CRT)){
            Log-Write "POSIX-UEFI runtime library was not created: $POSIXUEFI_LIB or $POSIXUEFI_CRT"
			throw ''
        }
    }
}

# (Ensure-PosixUefiRuntime)

$CARGS = @(
	"-I", "$(Get-Location)/src/",
	"-I", $EFIPARENT,
	"-I", "$($EFIPARENT)/uefi",
	'-nostdlib', "-m$(if($ARCHITECTURE -eq 'x86_64'){'64'}else{'32'})", '-fdiagnostics-color=always',
	'-fshort-wchar', '-fno-strict-aliasing', '-ffreestanding', '-fno-stack-protector', '-fno-stack-check',
	'-Wno-builtin-declaration-mismatch', '-fpic', '-fPIC',

)

if($ARCHITECTURE -eq 'x86_64'){$CARGS += '-DHAVE_USE_MS_ABI', '-mno-red-zone', '-maccumulate-outgoing-args'}

$LARGS = @(
	'-nostdlib', '-shared', '-Bsymbolic',
	'-L', $POSIXUEFI_BUILD,
    '-I', $POSIXUEFI_BUILD,
	$POSIXUEFI_CRT,
	'-luefi',
	'-T', "$($POSIXUEFI_BUILD)/link.ld",
	"-Wl,-Map,$($MAPFILE)",
    "$($POSIXUEFI_BUILD)/crt0.o"
)

# Compile all UEFI .asm files
Get-ChildItem -Path @((Join-Path (Get-Location) "src/Boot/UEFI"), "$($EFIPARENT)/uefi") -Include "*.asm" -Recurse -File | ForEach-Object{
	$f = $_
	$cont = $true
	$ARCHEXCLUDE | ForEach-Object{if($f.BaseName -match $_){$cont = $false}}
	if($cont){
		$o = Join-Path $Objdir "UEFI.c.$($_.BaseName).o"
		$tsInfo = Get-JsonTimestampInfo -JsonPath $TEMPJSON -FilePath $f.FullName
		$needsCompile = (-not $tsInfo.StoredTimestamp) -or (-not $tsInfo.StoredTime) -or ($tsInfo.ActualTime -gt $tsInfo.StoredTime)
		if($needsCompile){
			Log-Write -Msg "Stale source detected for $($f.FullName). source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Yellow
			Log-Write -Msg "$($NASM) -f $($ASM_FORMAT) $($_) -o $($o)" -color Blue
			$GCCOUT = & $NASM '-f' $ASM_FORMAT $_.FullName '-o' $o 2>&1
			Log-Write -Msg ($GCCOUT -join "`n")
			if(-not (Test-Path $o)){exit 1}
			Log-Write -Msg "Compiled and updated timestamp for $($f.FullName)" -color Green
			Update-JsonTimestamp -JsonPath $TEMPJSON -FilePath $f.FullName
		}else{Log-Write -Msg "Skipping $($f.FullName) - up to date; source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp)" -color Green}
		$Objects += $o
	}
}

# Compile all UEFI .c files
Get-ChildItem -Path @((Join-Path (Get-Location) "src/Boot/UEFI"), "$($EFIPARENT)/uefi") -Include "*.c" -Recurse -File | ForEach-Object{
	$f = $_
	$cont = $true
	$ARCHEXCLUDE | ForEach-Object{if($f.BaseName -match $_){$cont = $false}}
	if($cont){
		$o = Join-Path $Objdir "UEFI.c.$($_.BaseName).o"
		$tsInfo = Get-JsonTimestampInfo -JsonPath $TEMPJSON -FilePath $f.FullName
		$needsCompile = (-not $tsInfo.StoredTimestamp) -or (-not $tsInfo.StoredTime) -or ($tsInfo.ActualTime -gt $tsInfo.StoredTime)
		if($needsCompile){
			Log-Write -Msg "Stale source detected for $($f.FullName). source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Yellow
			Log-Write -Msg "$($GCC) -c $($_.FullName) -o $($o) $($CARGS -join ' ')" -color Blue
			$GCCOUT = & $GCC '-c' "$($_.FullName)" '-o' $o $CARGS 2>&1
			Log-Write -Msg ($GCCOUT -join "`n")
			if(-not (Test-Path $o)){exit 1}
			Log-Write -Msg "Compiled and updated timestamp for $($f.FullName)" -color Green
			Update-JsonTimestamp -JsonPath $TEMPJSON -FilePath $f.FullName
		}else{Log-Write -Msg "Skipping $($f.FullName) - up to date; source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp)" -color Green}
		$Objects += $o
	}
}

# Update timestamps for header files
Get-ChildItem -Path @((Join-Path (Get-Location) "src/Boot/UEFI"), "$($EFIPARENT)/uefi/") -Include "*.h" -Recurse -File | ForEach-Object{
	$f = $_
	$ARCHEXCLUDE | ForEach-Object{if($f.BaseName -match $_){$cont = $true}}
	if(-not $cont){
		if(Test-JsonTimestampIsOutdated -JsonPath $TEMPJSON -FilePath $f.FullName){
			$tsInfo = Get-JsonTimestampInfo -JsonPath $TEMPJSON -FilePath $f.FullName
			Update-JsonTimestamp -JsonPath $TEMPJSON -FilePath $f.FullName
			Log-Write -Msg "Updated timestamp for header file: $($f.FullName); source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Yellow
		}
	}
}

# Build blob
$SO_FILE = Join-Path $Objdir "blob.so"
Log-Write -Msg "$($LD) $($LARGS -join ' ') -o $($SO_FILE) $($Objects -join ' ')" -color Blue
$GCCOUT = & $LD $LARGS '-o', $SO_FILE, $Objects 2>&1
Log-Write -Msg ($GCCOUT -join "`n")
if($LASTEXITCODE -ne 0){exit 1}

# Convert to EFI
$EFIARCH = "efi-app-$ARCHITECTURE"
Log-Write -Msg "$($OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rela -j .rel -j .rela.* -j .reloc --target $($EFIARCH) --subsystem=10 $($SO_FILE) $($UEFIBINARYBLOB)" -color Blue
$OBJCOPYOUT = & $OBJCOPY '-j', '.text', '-j', '.sdata', '-j', '.data', '-j', '.dynamic', '-j', '.dynsym', '-j', '.rela', '-j', '.rel', '-j', '.rela.*', '-j', '.reloc', '--target', $EFIARCH, '--subsystem=10', $SO_FILE, $UEFIBINARYBLOB 2>&1
Log-Write -Msg ($OBJCOPYOUT -join "`n")
if($LASTEXITCODE -ne 0){exit 1}
Remove-Item $SO_FILE -Force

Copy-Item -Path $UEFIBINARYBLOB -Destination $EMUUEFIBINARYBLOB
Log-Write -Msg "Copied UEFI blob to $EMUUEFIBINARYBLOB" -color Cyan