param(
	# Feature Flags (Switches)
	[switch]$JsonEnabled, 
	[switch]$CacheEnabled, 
	[switch]$LogEnabled, 
	[switch]$DebugEnabled, 

	# Target & Compiler Arguments
	[Parameter(Mandatory = $false)][string[]]$f = @(), 
	[Parameter(Mandatory = $true)][string]$o, 
	[Parameter(Mandatory = $true)][string[]]$c = @(), 
	[Parameter(Mandatory = $true)][string[]]$l = @(), 
	[Parameter(Mandatory = $false)][string]$CACHEDIR = (Join-Path (Get-Location) 'compile/cache'), 
	[Parameter(Mandatory = $false)][string]$LogFile = (Join-Path (Get-Location) 'compile/log')
)

$NASM = 'nasm'
$LD = 'ld'

$_COMPILEARGS = @('-s'##'-nostdlib', '-O0', 
    ##'--std=c99', '-ffreestanding', 
    ##'-fno-builtin', '-funsigned-bitfields', 
    ##'-funsigned-char', '-fsso-struct=little-endian', 
    ##'-fdiagnostics-color=always', '-fdiagnostics-urls=always', 
    ##'-fno-diagnostics-show-highlight-colors', '-fomit-frame-pointer', 
    ##'-foptimize-crc', '-foptimize-strlen', '-finline-atomics', 
    ##'-fno-inline'
)
if($DebugEnabled){$_COMPILEARGS += '-g'}
if($LogEnabled){$_COMPILEARGS += @('-l', $LogFile)}
foreach($arg in $c){$_COMPILEARGS += $arg}

$_LINKARGS = @(
	'-ffreestanding', '-nostartfiles', '-nostdlib',
	'-shared', '-Bsymbolic', '-nodefaultlibs', 
	'-Wl,--emit-relocs', '-Wl,--relocatable'
)
foreach($arg in $l){$_LINKARGS += $arg}
$_LINKARGS += @('-o', $o)

function Get-TimestampCache{
	param(
		[Parameter(Mandatory = $true)]
		[string]$JsonPath
	)
	if($CacheEnabled){
		if(-not (Test-Path $JsonPath)){
			New-Item -Path $JsonPath -ItemType File -Force | Out-Null
			Set-Content -Path $JsonPath -Value '{}' -NoNewline
		}
	
		$raw = Get-Content -Path $JsonPath -Raw
		if([string]::IsNullOrWhiteSpace($raw)){
			$raw = '{}'
			Set-Content -Path $JsonPath -Value $raw -NoNewline
		}
		try{return $raw | ConvertFrom-Json -AsHashTable}catch{return @{}}
	}else{return @{}}
}

function Save-TimestampCache{
	param(
		[Parameter(Mandatory = $true)]
		[string]$JsonPath,
		[Parameter(Mandatory = $true)]
		$Data
	)
	if($CacheEnabled){$Data | ConvertTo-Json -Depth 4 | Set-Content -Path $JsonPath}
}

function Test-JsonTimestampIsOutdated{
	param(
		[Parameter(Mandatory = $true)]
		[string]$JsonPath,
		[Parameter(Mandatory = $true)]
		[string]$FilePath
	)
	if(-not $CacheEnabled){return $false}
	if(-not (Test-Path $FilePath)){throw "Target file not found: $FilePath"}

	$json = Get-TimestampCache -JsonPath $JsonPath
	$fullPath = (Get-Item $FilePath).FullName
	$storedTimestamp = if ($json.ContainsKey($fullPath)) { $json[$fullPath] } else { $null }
	$storedTime = $null
	if(-not $storedTimestamp){return $true}
	try {
		if($storedTimestamp.GetType() -eq [datetime]){$storedTime = $storedTimestamp
		}else{$storedTime = [DateTime]::Parse($storedTimestamp).ToUniversalTime()}
	}catch{return $true}
	$actualTime = (Get-Item $FilePath).LastWriteTimeUtc
	if($actualTime -gt $storedTime){return $true}

	return $false
}

function Get-JsonTimestampInfo{
	param(
		[Parameter(Mandatory = $true)]
		[string]$JsonPath,
		[Parameter(Mandatory = $true)]
		[string]$FilePath
	)
	if(-not (Test-Path $FilePath)){throw "Target file not found: $FilePath"}

	$json = Get-TimestampCache -JsonPath $JsonPath
	$fullPath = (Get-Item $FilePath).FullName
	$storedTimestamp = if ($json.ContainsKey($fullPath)) { $json[$fullPath] } else { $null }
	$storedTime = $null
	
	if($storedTimestamp){
		try{$storedTime = [DateTime]::Parse($storedTimestamp).ToUniversalTime()}catch{$storedTime = $null}
	}
	$actualTime = (Get-Item $FilePath).LastWriteTimeUtc
	return [PSCustomObject]@{
		FilePath        = $fullPath
		StoredTimestamp = $storedTimestamp
		StoredTime      = $storedTime
		ActualTime      = $actualTime
		Delta           = if ($storedTime) { $actualTime - $storedTime } else { $null }
	}
}

function Format-TimestampDiff{
	param(
		[Nullable[TimeSpan]]$Delta
	)
	return if($Delta){return 'n/a'}else{"{0:+0.000;-0.000;0.000}" -f $Delta.TotalSeconds}
}

function Update-JsonTimestamp{
	param(
		[Parameter(Mandatory = $true)]
		[string]$JsonPath,
		[Parameter(Mandatory = $true)]
		[string]$FilePath
	)
	if($CacheEnabled){
		if(-not (Test-Path $FilePath)){throw "Target file not found: $FilePath"}
		$json = Get-TimestampCache -JsonPath $JsonPath
		$fullPath = (Get-Item $FilePath).FullName
		$json[$fullPath] = (Get-Item $FilePath).LastWriteTimeUtc.ToString('o')
		Save-TimestampCache -JsonPath $JsonPath -Data $json
	}
}

function LogWrite{
	param([string]$Message, [ConsoleColor]$Color = [ConsoleColor]::White)
	if($LogFile -and $LogEnabled){
		if(-not (Test-Path $LogFile)){New-Item $LogFile -ItemType File -Force | Out-Null}
		Add-Content -Path $LogFile -Value $Message
	}
	Write-Host $Message -ForegroundColor $Color
}

# if($LogFile -and (Test-Path $LogFile)){Remove-Item $LogFile -Force}

# Logging Helper
LogWrite "Initializing CNASM.MAKE build process..."
if($DebugEnabled){
	LogWrite "[DEBUG] Sources: $($f -join ', ')" Gray
	LogWrite "[DEBUG] Output Target: $o" Gray
	LogWrite "[DEBUG] Compile Flags: $($_COMPILEARGS -join ' ')" Gray
	LogWrite "[DEBUG] Link Flags: $($_LINKARGS -join ' ')" Gray
}
$objectFiles = @()
if($CacheEnabled){
	if(-not (Test-Path $CACHEDIR)){New-Item $CACHEDIR -ItemType Directory -Force | Out-Null}
	$jsonCachePath = Join-Path $CACHEDIR 'cache.json'

	foreach($srcPath in $f){
		$srcItem = Get-Item $srcPath
		$objPath = Join-Path $CACHEDIR "$($srcItem.BaseName).o"
		$tsInfo = Get-JsonTimestampInfo -JsonPath $jsonCachePath -FilePath $srcItem.FullName
		if(Test-JsonTimestampIsOutdated -JsonPath $jsonCachePath -FilePath $srcItem.FullName){
			$diffStr = Format-TimestampDiff -Delta $tsInfo.Delta
			LogWrite "Stale source detected for $($srcItem.FullName). source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$diffStr sec" Yellow
			$compileCmd = @() + $_COMPILEARGS + $srcItem.FullName + '-o' + $objPath
			LogWrite "$NASM $($compileCmd -join ' ')"
			$NASMOut = (& $NASM @compileCmd) 2>&1
			if($NASMOut){LogWrite ($NASMOut -join "`n")}
			if(-not (Test-Path $objPath)){LogWrite "Failure in compiling $($srcItem.FullName) -> $objPath" Red
				exit 1
			}
			LogWrite "Compiled and updated timestamp for $($srcItem.FullName)" Green
			Update-JsonTimestamp -JsonPath $jsonCachePath -FilePath $srcItem.FullName
		}else{LogWrite "Skipping $($srcItem.FullName) - up to date; source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp)" Green}
		$objectFiles += $objPath
	}
	$linkCmd = @() + $_LINKARGS + $objectFiles + '-o' + $o
	LogWrite "$LD $($linkCmd -join ' ')"
	$linkOut = (& $LD @linkCmd) 2>&1
	if($linkOut){LogWrite ($linkOut -join "`n")}
}else{
	# Direct compilation without caching
	$directCmd = @() + $_COMPILEARGS + $f + '-o' + $o + $_LINKARGS
	$fullCmdStr = "$LD " + ($directCmd -join " ")

	if($JsonEnabled){
		$buildManifest = [PSCustomObject]@{
			Command      = "CNASM.MAKE"
			CacheEnabled = $false
			DebugEnabled = [bool]$DebugEnabled
			Sources      = $f
			Output       = $o
			CompileFlags = $_COMPILEARGS
			LinkFlags    = $_LINKARGS
			NASMCommand   = $fullCmdStr
		}
		Write-Host ($buildManifest | ConvertTo-Json -Depth 3)
	}

	LogWrite "Executing: $fullCmdStr"
	$LDOut = (& $LD @directCmd) 2>&1
	if($NASMOut){LogWrite ($LDOut -join "`n")}
}

if($LASTEXITCODE -ne 0){LogWrite "Build failed with exit code $LASTEXITCODE" Red
}else{LogWrite "Build finished successfully -> $o" Green}
exit $LASTEXITCODE