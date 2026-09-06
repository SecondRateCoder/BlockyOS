# param(
#     [string[]]$INPUTFILES,
#     [string]$OUTPUTFILE,
#     [string[]]$COMPILEARGS,
#     [string[]]$LINKARGS,
#     [string]$LOGFILE,
#     [switch]$CACHE,
#     [switch]$DEBUG
# )

# if($LOGFILE){if(Test-Path $LOGFILE){Remove-Item $LOGFILE -Force}}
# $GCC = 'gcc'

# $_COMPILEARGS = @(
#     '-nostdlib'
# )
# if($DEBUG){$_COMPILEARGS += '-g'}
# $_LINKARGS = @(
#     '-ffreestanding', '-nostartfiles', '-nostdlib', 
#     '-shared', '-Bsymbolic', 
# 	'-Wl,--emit-relocs', '-Wl,--relocatable',
#     '-nostartfiles', '-nodefaultlibs'
# )

# function Log-Write{
#     param([string]$MESSAGE)
#     if($LOGFILE){
#         if(-not (Test-Path $LOGFILE)){New-Item $LOGFILE -ItemType File -Force}
#         Add-Content -Path $LOGFILE -Value $MESSAGE
#     }
#     Write-Host $MESSAGE
# }

# if($CACHE){
#     $CACHEDIR = Join-Path (Get-Location) 'tools/build-suite/cache/'
#     if(-not (Test-Path $CACHEDIR)){New-Item $CACHEDIR -ItemType Directory -Force}
#     $JSONCACHE = Join-Path $CACHEDIR 'cache.json'

#     function Get-TimestampCache{
#         param(
#             [Parameter(Mandatory)]
#             [string]$JsonPath
#         )

#         if(-not (Test-Path $JsonPath)){
#             New-Item -Path $JsonPath -ItemType File -Force | Out-Null
#             Set-Content -Path $JsonPath -Value '{}' -NoNewline
#         }

#         $raw = Get-Content -Path $JsonPath -Raw
#         if([string]::IsNullOrWhiteSpace($raw)){
#             $raw = '{}'
#             Set-Content -Path $JsonPath -Value $raw -NoNewline
#         }

#         try{return $raw | ConvertFrom-Json -AsHashTable
#         }catch{return @{}}
#     }

#     function Save-TimestampCache{
#         param(
#             [Parameter(Mandatory)]
#             [string]$JsonPath,
#             [Parameter(Mandatory)]
#             $Data
#         )
#         $Data | ConvertTo-Json -Depth 4 | Set-Content -Path $JsonPath
#     }

#     function Test-JsonTimestampIsOutdated{
#         param(
#             [Parameter(Mandatory)]
#             [string]$JsonPath,
#             [Parameter(Mandatory)]
#             [string]$FilePath
#         )

#         if(-not(Test-Path $FilePath)){throw "Target file not found: $FilePath"}

#         $json = Get-TimestampCache -JsonPath $JsonPath
#         $fullPath = (Get-Item $FilePath).FullName

#         $storedTimestamp = if($json.ContainsKey($fullPath)){$json[$fullPath]}else{$null}
#         if(-not $storedTimestamp){return $true}

#         $storedTime = $null
#         try{
#             if($storedTimestamp.GetType() -eq [datetime]){
#                 $storedTime = $storedTimestamp
#             }else{$storedTime = [DateTime]::Parse($storedTimestamp).ToUniversalTime()}
#         }catch{return $true}

#         $actualTime = (Get-Item $FilePath).LastWriteTime.ToUniversalTime()
#         if($actualTime -gt $storedTime.ToUniversalTime()){return $true}

#         return $false
#     }

#     function Get-JsonTimestampInfo {
#         param(
#             [Parameter(Mandatory)]
#             [string]$JsonPath,
#             [Parameter(Mandatory)]
#             [string]$FilePath
#         )

#         if(-not (Test-Path $FilePath)) { throw "Target file not found: $FilePath" }

#         $json = Get-TimestampCache -JsonPath $JsonPath
#         $fullPath = (Get-Item $FilePath).FullName
#         $storedTimestamp = if ($json.ContainsKey($fullPath)) { $json[$fullPath] } else { $null }
#         $storedTime = $null
#         if($storedTimestamp){
#             try{ $storedTime = [DateTime]::Parse($storedTimestamp).ToUniversalTime() } catch { $storedTime = $null }
#         }
#         $actualTime = (Get-Item $FilePath).LastWriteTimeUtc

#         return [PSCustomObject]@{
#             FilePath = $fullPath
#             StoredTimestamp = $storedTimestamp
#             StoredTime = $storedTime
#             ActualTime = $actualTime
#             Delta = if($storedTime){ $actualTime - $storedTime } else { $null }
#         }
#     }

#     function Format-TimestampDiff{
#         param(
#             [Nullable[TimeSpan]]$Delta
#         )

#         if(-not $Delta){ return 'n/a' }
#         return "{0:+0.000;-0.000;0.000}" -f $Delta.TotalSeconds
#     }

#     function Update-JsonTimestamp{
#         param(
#             [Parameter(Mandatory)]
#             [string]$JsonPath,
#             [Parameter(Mandatory)]
#             [string]$FilePath
#         )
#         if(-not (Test-Path $FilePath)) { throw "Target file not found: $FilePath" }

#         $json = Get-TimestampCache -JsonPath $JsonPath
#         $fullPath = (Get-Item $FilePath).FullName
#         $json[$fullPath] = (Get-Item $FilePath).LastWriteTimeUtc.ToString('o')
#         Save-TimestampCache -JsonPath $JsonPath -Data $json
#     }

#     $OFILES = @()
#     $INPUTFILES | ForEach-Object{
#         $f = Get-Item $_
#         $o = Join-Path $CACHEDIR "$($_.BaseName).o"
#         $tsInfo = Get-JsonTimestampInfo -JsonPath $CACHEDIR -FilePath $f.FullName
#         if(Test-JsonTimestampIsOutdated -JsonPath $CACHEDIR -FilePath $f.FullName){
#             Log-Write -Msg "Stale source detected for $($f.FullName). source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Yellow
#             Log-Write "$($GCC) $($_COMPILEARGS) $($COMPILEARGS) -o $($OUTPUTFILE)"
#             $GCCOUT = (& $GCC $_COMPILEARGS $COMPILEARGS '-o' $OUTPUTFILE) 2>&1
#             Log-Write -Msg ($GCCOUT -join "`n")
#             if(-not (Test-Path $o)){
#                 Log-Write "Failure In Compiling $($f.FullName)  :  $($o)" -color Red
#                 exit 1
#             }
#             Log-Write -Msg "Compiled and updated timestamp for $($f.FullName)" -color Green
#             Update-JsonTimestamp -JsonPath $CACHEDIR -FilePath $f.FullName
#             Copy-Item -Path $o -Destination (Join-Path $TEMPCACHE "$($_.BaseName).o")
#         }else{
#             Log-Write -Msg "Skipping $($f.FullName) - up to date; source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp)" -color Green
#             Copy-Item -Path (Join-Path $CACHEDIR "$($_.BaseName).o") -Destination $o
#         }
#         $OFILES += $o
#     }
#     Log-Write "$GCC $($_LINKARGS -join "`n") $($LINKARGS -join "`n") $($OFILES -join "`n") -o $($OUTPUTFILE -join "`n")"
#     $GCCOUT = (& $GCC $_LINKARGS $LINKARGS $OFILES '-o' $OUTPUTFILE) 2>&1
#     Log-Write ($GCCOUT -JOIN "`n")
# }else{
#     Log-Write "$($GCC) $($_COMPILEARGS) $($COMPILEARGS) -o $($OUTPUTFILE)"
#     $GCCOUT = (& $GCC $_COMPILEARGS $COMPILEARGS '-o' $OUTPUTFILE) 2>&1
#     Log-Write ($GCCOUT -join "`n")
# }

# function MAKE{
#     [CmdletBinding()]
#     param(
#         # Feature Flags (Switches)
#         [switch]$JsonEnabled, 
#         [switch]$CacheEnabled, 
#         [switch]$LogEnabled, 
#         [switch]$DebugEnabled, 
#         [Parameter(Mandatory = $false)][string[]]$f = @(), 
#         [Parameter(Mandatory = $true)][string]$o, 
#         [Parameter(Mandatory = $true)][string[]]$c = @(), 
#         [Parameter(Mandatory = $true)][string[]]$l = @()
#     )

#     # Logging Helper
#     $LogMessage = {
#         param([string]$Message, [string]$Level = "INFO")
#         if($Log -or $Debug){
#             $timestamp = Get-Date -Format "HH:mm:ss"
#             Write-Host "[$timestamp] [$Level] $Message"
#         }
#     }

#     & $LogMessage "Initializing CGCC.MAKE..."

#     # Feature: DEBUG
#     if($Debug){
#         & $LogMessage "Debug Mode Enabled." "DEBUG"
#         & $LogMessage "Sources: $($f -join ', ')" "DEBUG"
#         & $LogMessage "Output:  $o" "DEBUG"
#         & $LogMessage "Compile Flags: $($c -join ' ')" "DEBUG"
#         & $LogMessage "Link Flags:    $($l -join ' ')" "DEBUG"
#     }

#     # Feature: FSFRAT.RUNTIME Validation
#     if ($FsFratRuntime) {
#         & $LogMessage "Checking source file existence..." "DEBUG"
#         foreach ($file in $f) {
#             if (-not (Test-Path -Path $file)) {
#                 $err = "Source file not found: $file"
#                 & $LogMessage $err "ERROR"
#                 throw $err
#             }
#         }
#     }

#     # Feature: CACHE
#     if ($Cache) {
#         & $LogMessage "Cache enabled: Checking build target freshness..." "INFO"
#         if ((Test-Path $o) -and ($f.Count -gt 0)) {
#             $outputTime = (Get-Item $o).LastWriteTime
#             $stale = $false
#             foreach ($file in $f) {
#                 if ((Get-Item $file).LastWriteTime -gt $outputTime) {
#                     $stale = $true
#                     break
#                 }
#             }
#             if (-not $stale) {
#                 & $LogMessage "Output '$o' is up-to-date. Skipping compilation (Cache Hit)." "INFO"
#                 return
#             }
#         }
#     }

#     # Construct arguments for GCC
#     # Format: gcc [compile flags] [sources] -o [output] [link flags]
#     $gccArgs = @()
#     $gccArgs += $c
#     $gccArgs += $f
#     $gccArgs += "-o"
#     $gccArgs += $o
#     $gccArgs += $l

#     $fullCommand = "gcc " + ($gccArgs -join " ")

#     # Feature: JSON Output
#     if ($Json) {
#         $buildInfo = [PSCustomObject]@{
#             Command      = "CGCC.MAKE"
#             CacheEnabled = [bool]$Cache
#             DebugEnabled = [bool]$Debug
#             Sources      = $f
#             Output       = $o
#             CompileFlags = $c
#             LinkFlags    = $l
#             GCCInvocation= $fullCommand
#         }
#         Write-Host ($buildInfo | ConvertTo-Json -Depth 3)
#     }

#     # Execute GCC
#     & $LogMessage "Running: $fullCommand" "INFO"
#     & gcc @gccArgs

#     if ($LASTEXITCODE -ne 0) {
#         & $LogMessage "GCC process failed with exit code $LASTEXITCODE." "ERROR"
#         exit $LASTEXITCODE
#     } else {
#         & $LogMessage "Build completed successfully -> $o" "INFO"
#     }
# }