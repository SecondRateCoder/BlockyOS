param(
	# Feature Flags (Switches)
	[switch]$CacheEnabled, 
	[switch]$LogEnabled, 
	[switch]$DebugEnabled, 
	[switch]$JsonEnabled, # Added missing parameter

	# Target & Compiler Arguments
	[Parameter(Mandatory = $true)][string[]]$f = @(), 
	[Parameter(Mandatory = $true)][string]$o, 
	[Parameter(Mandatory = $false)][string[]]$c = @(), 
	[Parameter(Mandatory = $false)][string[]]$l = @(), 
	[Parameter(Mandatory = $false)][string]$CACHEDIR = (Join-Path (Get-Location) 'compile/cache'), 
	[Parameter(Mandatory = $false)][string]$LogFile = (Join-Path (Get-Location) 'compile/compile.log')
)
$envOLDPATH = $env:PATH
$ROOT = @(
    (Join-Path (Get-Location) "compile/toolchain/prebuild/x86_64-elf/bin/"),
    (Join-Path (Get-Location) "compile/toolchain/prebuild/x86_64-elf/x86_64-elf/bin/")
)
$env:PATH = "$($ROOT -join ';');$($env:PATH)"
$env:COMPILER_PATH = $ROOT[0]
$GCC = 'gcc'
$NASM = 'nasm'

$PATTERN = '"[^"]*"|''[^'']*''|\{[^{}]*\}|\[[^\[\]]*\]|\([^\(\)]*\)|\S+'

$_COMPILEARGS = @('-nostdlib', '-O0', 
    '--std=c99', '-ffreestanding', '-m64', '-mno-red-zone', 
    '-fno-stack-protector', '-fno-builtin', '-funsigned-bitfields', 
    '-funsigned-char', '-fsso-struct=little-endian', '-fno-leading-underscore', 
    '-fdiagnostics-color=always', '-fdiagnostics-urls=always', 
    '-fno-diagnostics-show-highlight-colors', '-fomit-frame-pointer', 
    '-foptimize-crc', '-foptimize-strlen', '-finline-atomics', 
    '-fno-inline'
)
if($DebugEnabled){$_COMPILEARGS += '-g'}
foreach($arg in $c){[regex]::Matches($arg, $PATTERN) | ForEach-Object{$_COMPILEARGS += $_.ToString()}}

$_LINKARGS = @('-fdiagnostics-color=always', '-fno-diagnostics-show-highlight-colors', '-nostartfiles', '-nodefaultlibs', '-flinker-output=dyn', '-nostdlib', '--emit-relocs')
$_LINKSKIP = 4
foreach($arg in $l){[regex]::Matches($arg, $PATTERN) | ForEach-Object{$_LINKARGS += $_.ToString()}}
if($LogEnabled){
    $fullPath = [System.IO.Path]::GetFullPath($LogFile)
    $logDir = [System.IO.Path]::GetDirectoryName($fullPath)
    $_LINKARGS += @('-Map', (Join-Path $logDir 'alibcrt.map'))
}
$_LINKARGS += @('-o', $o)

function Get-TimestampCache {
	param([Parameter(Mandatory = $true)][string]$JsonPath)
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

function Save-TimestampCache {
	param(
		[Parameter(Mandatory = $true)][string]$JsonPath,
		[Parameter(Mandatory = $true)]$Data
	)
	if($CacheEnabled){$Data | ConvertTo-Json -Depth 4 | Set-Content -Path $JsonPath}
}

function Test-JsonTimestampIsOutdated {
	param(
		[Parameter(Mandatory = $true)][string]$JsonPath,
		[Parameter(Mandatory = $true)][string]$FilePath
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

function Get-JsonTimestampInfo {
	param(
		[Parameter(Mandatory = $true)][string]$JsonPath,
		[Parameter(Mandatory = $true)][string]$FilePath
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

function Format-TimestampDiff {
	param([Nullable[TimeSpan]]$Delta)
	if(-not $Delta){return 'n/a'}else{return "{0:+0.000;-0.000;0.000}" -f $Delta.TotalSeconds}
}

function Update-JsonTimestamp {
	param(
		[Parameter(Mandatory = $true)][string]$JsonPath,
		[Parameter(Mandatory = $true)][string]$FilePath
	)
	if($CacheEnabled){
		if(-not (Test-Path $FilePath)){throw "Target file not found: $FilePath"}
		$json = Get-TimestampCache -JsonPath $JsonPath
		$fullPath = (Get-Item $FilePath).FullName
		$json[$fullPath] = (Get-Item $FilePath).LastWriteTimeUtc.ToString('o')
		Save-TimestampCache -JsonPath $JsonPath -Data $json
	}
}

function LogWrite {
	param([string]$Message, [ConsoleColor]$Color = [ConsoleColor]::White)
	if($LogFile -and $LogEnabled){
	    $esc=[char]27
		if(-not (Test-Path $LogFile)){New-Item $LogFile -ItemType File -Force | Out-Null}
		Add-Content -Path $LogFile -Value ($Message -replace "$esc(?:\[[0-9;?]*[ -/]*[@-~]|][^\a]*\a|P.*?$esc\\|X.*?$esc\\|\^.*?$esc\\|_.*?$esc\\|[@-Z\\-_])","")
	}
	Write-Host $Message -ForegroundColor $Color
}

function Format-LinkerArgs{
	param([string[]]$ArgsList)
	$processedArgs = @()
	for($i = $_LINKSKIP; $i -lt ($ArgsList.Count - 2); $i++){
		$item = $null
		if($ArgsList[$i][0] -eq '-'){
			$item += "-Wl,$($ArgsList[$i])"
			while(($ArgsList[$i + 1][0] -ne '-') -and ($i -lt ($ArgsList.Count - 2))){
				$item += ",$($ArgsList[$i + 1])"
				$i++
			}
		}
		$processedArgs += $item
	}
	$processedArgs += $ArgsList[0..($_LINKSKIP - 1)]
	$processedArgs += @('-o', $o)
	return $processedArgs
}

LogWrite "Initializing CGCC.MAKE build process..."
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
		$objPath = if(($srcItem.Extension -match 'asm') -or ($srcItem.Extension -match 'c')){
			Join-Path $CACHEDIR "$($srcItem.Extension -replace '\.','').$($srcItem.BaseName).o"}else{$srcPath}
		$cc = 0
		while($objectFiles -contains $objPath){
			$cc++
			$objPath = if(($srcItem.Extension -match 'asm') -or ($srcItem.Extension -match 'c')){
			Join-Path $CACHEDIR "$($srcItem.Extension -replace '\.','').$($srcItem.BaseName)($cc).o"}else{exit 1}
		}
		$tsInfo = Get-JsonTimestampInfo -JsonPath $jsonCachePath -FilePath $srcItem.FullName
		if(Test-JsonTimestampIsOutdated -JsonPath $jsonCachePath -FilePath $srcItem.FullName){
			$diffStr = Format-TimestampDiff -Delta $tsInfo.Delta
			LogWrite "Stale source detected for $($srcItem.FullName). source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$diffStr sec" Yellow
			$compileCmd = @('-c', $srcItem.FullName, '-o', $objPath.ToString()) + $_COMPILEARGS
			switch -Regex ($srcItem.Extension){
				'asm' {
					$nasmCMD = @('-s')
					if($DebugEnabled){$nasmCMD += '-g'}
					# if($LogEnabled){$nasmCMD += '-l', $LogFile}
					for($cc = 0; $cc -lt $_COMPILEARGS.Count; ++$cc){
						switch -Regex ($_COMPILEARGS[$cc]){
							'^-I$' {
								if(($cc + 1) -lt $_COMPILEARGS.Count){$cc++;	$nasmCMD += @('-I', $_COMPILEARGS[$cc])}
								break
							} '^-D$' {
								if(($cc + 1) -lt $_COMPILEARGS.Count){$cc++;	$nasmCMD += @('-D', $_COMPILEARGS[$cc])}
								break
							} 
							'^-m64$'					{$nasmCMD += @('-f', 'win64');				break} 
							'^-m32$'					{$nasmCMD += @('-f', 'obj');				break} 
							'^-O([0-3sgfast]|\w+)?$'	{$nasmCMD += $_COMPILEARGS[$cc];			break}
						}
					}
					#	Default to 64-bit Obj format. 
					if($nasmCMD -notcontains 'win64' -and $nasmCMD -notcontains 'obj'){$nasmCMD += @('-f', 'win64')}
					$nasmCMD += @($srcPath, '-o', $objPath)
					LogWrite "$NASM $($nasmCMD -join ' ')" Yellow
					
					# Ensure NASM always executes regardless of logging
					$NASMOut = (& $NASM @nasmCMD 2>&1)
					if($NASMOut){LogWrite ($NASMOut -join "`n")}
				} 'c' {
					LogWrite "$GCC $($compileCmd -join ' ')" Yellow
					$gccOut = (& $GCC @compileCmd 2>&1)
					if($gccOut){LogWrite ($gccOut -join "`n")}
				}
			}
			if(-not (Test-Path $objPath)){
				LogWrite "Failure in compiling $($srcItem.FullName) -> $objPath" Red
				$env:COMPILER_PATH = $null
				$env:PATH = $envOLDPATH
				exit 1
			}
			LogWrite "Compiled and updated timestamp for $($srcItem.FullName)" Green
			Update-JsonTimestamp -JsonPath $jsonCachePath -FilePath $srcItem.FullName
		}else{LogWrite "Skipping $($srcItem.FullName) - up to date; source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp)" Green}
		$objectFiles += $objPath
	}
	
	$LINKARGS = Format-LinkerArgs -ArgsList $_LINKARGS
	$linkCmd = @() + $objectFiles + $LINKARGS
	LogWrite "$GCC $($linkCmd -join ' ')" Yellow
	$linkOut = (& $GCC @linkCmd 2>&1)
	if($linkOut){LogWrite ($linkOut -join "`n")}
}else{
	# Direct compilation without caching
	$LINKARGS = Format-LinkerArgs -ArgsList $_LINKARGS
	$directCmd = @() + $_COMPILEARGS + $f + $LINKARGS
	$fullCmdStr = "$GCC " + ($directCmd -join " ")

	if($JsonEnabled){
		$buildManifest = [PSCustomObject]@{
			Command      = "CGCC.MAKE"
			CacheEnabled = $false
			DebugEnabled = [bool]$DebugEnabled
			Sources      = $f
			Output       = $o
			CompileFlags = $_COMPILEARGS
			LinkFlags    = $LINKARGS
			GCCCommand   = $fullCmdStr
		}
		Write-Host ($buildManifest | ConvertTo-Json -Depth 3)
	}
	LogWrite "Executing: $fullCmdStr" Yellow
	$gccOut = (& $GCC @directCmd 2>&1)
	if($gccOut){LogWrite ($gccOut -join "`n")}
}

if($LASTEXITCODE -ne 0){LogWrite "Build failed with exit code $LASTEXITCODE" Red
}else{LogWrite "Build finished successfully -> $o" Green}
$env:COMPILER_PATH = $null
$env:PATH = $envOLDPATH
exit $LASTEXITCODE