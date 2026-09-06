param(
	[string]$PREFIX,
	[string]$NASM,
	[string]$GCC
)

$Build = Join-Path (Get-Location) ("Build/Build-" + $PREFIX)
$TEMPFOLDER = Join-Path (Get-Location) 'compile/cache'
if(-not (Test-Path $TEMPFOLDER)){New-Item $TEMPFOLDER -ItemType Directory -Force}
$TEMPJSON = Join-Path (Get-Location) 'compile/cache/desc.json'
if(-not (Test-Path $TEMPJSON)){New-Item $TEMPJSON -ItemType File -Force}
$Objdir = Join-Path $Build "objs"
$Log = Join-Path $Build ("st2.log")
$ST2BIN = Join-Path -Path $Objdir "boot2.bin"
$MAPFILE = Join-Path -Path $Build "boot2map.log"

$LINKERSCRIPT = Join-Path (Get-Location) "src/Boot/Legacy/stage2/boot.ld"

$ForceOrder = @(
    "math", "memory", "SHA256", "stdio", "IO", "stdkernel", "interrupt", "IRQ", "stdmem", "localfile", "f-rat", "stdprogram"
)

$EXCLUDE = @(
	"stdfile", "f-rat", "Devices", "USB", "PCI", "DeviceInterrupts"
)

$BOOT2CFILES = @()
$BOOT2ASMFILES = @()
$BOOT2OFILES = @()
$COMBINED = @()

New-Item -Path $Log -ItemType File -Force

function Log-Write{
    param(
        [string]$Msg,
        [System.ConsoleColor]$color
    )
    # $clean = $Msg -replace "$esc(?:\[[0-9;?]*[ -/]*[@-~]|][^\a]*\a|P.*?$esc\\|X.*?$esc\\|\^.*?$esc\\|_.*?$esc\\|[@-Z\\-_])",""
    $esc=[char]27
    $clean = $Msg -replace "$($esc)\[[0-9;]*[A-Za-z]",""
    if($color){Write-Host $Msg -ForegroundColor $color
    }else{Write-Host $Msg}
    $success = $false
    do{
        $success = $true
        try{
            if(-not (Test-Path $Log)){New-Item $Log -ItemType File}
            Add-Content -Path $Log -Value $clean
        }catch{$success = $true}
    }while($success -eq $false)
}

function Get-FolderPriority{
    param(
        [string]$FilePath,
        [string[]]$ForceOrder
    )

    # Normalize path separators
    $normalized = $FilePath -replace '\\','/'

    # Find all folder internalmatches and pick the deepest one
    $internalmatches = foreach ($i in 0..($ForceOrder.Count-1)) {
        if ($normalized -match [regex]::Escape($ForceOrder[$i])) {
            [PSCustomObject]@{
                Index = $i
                Depth = ($normalized.Split('/') | Where-Object { $_ -eq $ForceOrder[$i] }).Count
            }
        }
    }

    if($internalmatches){
        # Sort by folder priority first, then by depth (deepest match wins)
        return ($internalmatches | Sort-Object Index, @{Expression='Depth';Descending=$true})[0].Index
    }

    # No match → lowest priority
    return [int]::MaxValue
}

function Get-TimestampCache{
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

function Save-TimestampCache {
    param(
        [Parameter(Mandatory)]
        [string]$JsonPath,

        [Parameter(Mandatory)]
        $Data
    )

    $Data | ConvertTo-Json -Depth 4 | Set-Content -Path $JsonPath
}

function Test-JsonTimestampIsOutdated {
    param(
        [Parameter(Mandatory)]
        [string]$JsonPath,

        [Parameter(Mandatory)]
        [string]$FilePath
    )

    if(-not (Test-Path $FilePath)) { throw "Target file not found: $FilePath" }

    $json = Get-TimestampCache -JsonPath $JsonPath
    $fullPath = (Get-Item $FilePath).FullName

    $storedTimestamp = if ($json.PSObject.Properties.Name -contains $fullPath) { $json.$fullPath } else { $null }
    if(-not $storedTimestamp) {
        return $true
    }

    try {
        $storedTime = [DateTime]::Parse($storedTimestamp)
    } catch {
        return $true
    }

    $actualTime = (Get-Item $FilePath).LastWriteTimeUtc
    if($actualTime -gt $storedTime.ToUniversalTime()) {
        return $true
    }

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
    $storedTimestamp = if ($json.PSObject.Properties.Name -contains $fullPath) { $json.$fullPath } else { $null }
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
        Delta = if($storedTime){$actualTime - $storedTime}else{$null}
    }
}

function Format-TimestampDiff {
    param(
        [Nullable[TimeSpan]]$Delta
    )

    if(-not $Delta){ return 'n/a' }
    return "{0:+0.000;-0.000;0.000}" -f $Delta.TotalSeconds
}

function Update-JsonTimestamp {
    param(
        [Parameter(Mandatory)]
        [string]$JsonPath,

        [Parameter(Mandatory)]
        [string]$FilePath
    )

    if(-not (Test-Path $FilePath)) { throw "Target file not found: $FilePath" }

    $json = Get-TimestampCache -JsonPath $JsonPath
    $fullPath = (Get-Item $FilePath).FullName
    $json.$fullPath = (Get-Item $FilePath).LastWriteTimeUtc.ToString('o')
    Save-TimestampCache -JsonPath $JsonPath -Data $json
}

function Sort-ByForceOrder {
    param(
        [string[]]$Files,
        [string[]]$ForceOrder
    )

    return $Files | Sort-Object {
        Get-FolderPriority -FilePath $_ -ForceOrder $ForceOrder
    }
}


$COMPILECLI = @(
	"-nostdlib", "-m32", "-z", "nostartfiles",
	"-fdiagnostics-color=always",  "-fno-leading-underscore", "-ffreestanding", "-fno-stack-protector"
	"-I", "$(Join-Path (Get-Location) "src/")", 
	"-std=c99",
	"-D", "LOCALSTANDARDFILE", "-D", "LOCALFILE", '-D', '__ia32__'
)

# Order files
Log-Write -Msg "Files:" -color Blue
(Get-ChildItem -Path @((Join-Path (Get-Location) "src/Boot/Legacy/stage2/"), (Join-Path (Get-Location) "src/kernel/lib/")) -Include @("*.c", "*.asm") -Recurse -Force -File)|ForEach-Object{
    if($_ -ilike "*src/Boot/Legacy/stage2/*"){
        Log-Write -Msg " $($_)," -color Blue
        $COMBINED += $_.FullName
    }elseif($EXCLUDE -notcontains $_.BaseName){
        Log-Write -Msg " $($_)," -color Blue
        $COMBINED += $_.FullName
    }
}

Log-Write -Msg "Sorted:" -color Blue
(Sort-ByForceOrder -Files $COMBINED -ForceOrder $ForceOrder)|ForEach-Object{
    if($_ -match 'asm'){
        Log-Write -Msg " $($_)," -color Blue
        $BOOT2ASMFILES += $_
        $BOOT2OFILES += (Join-Path -Path $Objdir "asm.$([System.IO.Path]::GetFileNameWithoutExtension($_)).o")
    }
    elseif($_ -match '.c'){
        Log-Write -Msg " $($_)," -color Blue
        $BOOT2CFILES += $_
        $BOOT2OFILES += (Join-Path -Path $Objdir "c.$([System.IO.Path]::GetFileNameWithoutExtension($_)).o")
    }
}

# Compile all .c files
$BOOT2CFILES|ForEach-Object{
    $o = Join-Path -Path $Objdir "c.$((Get-Item -Path $_).BaseName).o"
    $cacheFile = Join-Path $TEMPFOLDER "$((Get-Item $_).BaseName).o"
    $tsInfo = Get-JsonTimestampInfo -JsonPath $TEMPJSON -FilePath $_
    $needsCompile = (-not $tsInfo.StoredTimestamp) -or ($tsInfo.ActualTime -gt $tsInfo.StoredTime) -or (-not (Test-Path $cacheFile))

    if($needsCompile){
        Log-Write -Msg "Stale source detected for $($_). source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Yellow
        Log-Write -Msg "$($GCC) -c $($_) -o $($o) $($COMPILECLI -join ' ')" -color Blue
        $COMPILEOUT = & $GCC '-c' $_ '-o' $o $COMPILECLI 2>&1
        if($LASTEXITCODE -ne 0){
            Log-Write -Msg "$($COMPILEOUT -join "`n")" -color Red
            throw "Compilation failed for $($_)"
        }
        Copy-Item -Path $o -Destination $cacheFile -Force
        Log-Write -Msg "Compiled and cached object: $cacheFile" -color Green
        Update-JsonTimestamp -JsonPath $TEMPJSON -FilePath $_
        Log-Write "$($COMPILEOUT -join "`n")"
    } else {
        Log-Write -Msg "Restoring cached object for $($_) from $cacheFile; source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Green
        Copy-Item -Destination $o -Path $cacheFile -Force
        Log-Write -Msg "Copied cache to build object: $o" -color Cyan
    }
}

# Compile .asm files
$BOOT2ASMFILES|ForEach-Object{
    $o = Join-Path -Path $Objdir "asm.$((Get-Item -Path $_).BaseName).o"
    $cacheFile = Join-Path $TEMPFOLDER "$((Get-Item $_).BaseName).o"
    $args_ = @("-f", "elf32", $_, "-o", $o, "-d", "__DEBUG=")
    $tsInfo = Get-JsonTimestampInfo -JsonPath $TEMPJSON -FilePath $_
    $needsCompile = (-not $tsInfo.StoredTimestamp) -or ($tsInfo.ActualTime -gt $tsInfo.StoredTime) -or (-not (Test-Path $cacheFile))

    if($needsCompile){
        Log-Write -Msg "Stale source detected for $($_). source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp ?? 'none'), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Yellow
        Log-Write -Msg "$($NASM) $($args_ -join ' ')" -color Blue
        $COMPILEOUT = (& $NASM $args_) 2>&1
        $COMPILEOUT|ForEach-Object{
            if($_ -match 'error'){Log-Write -Msg $_ -color Red}
            else{Log-Write -Msg $_ -color Yellow}
        }
        if($LASTEXITCODE -ne 0){throw "Assembler failed for $($_)"}
        Copy-Item -Path $o -Destination $cacheFile -Force
        Log-Write -Msg "Assembled and cached object: $cacheFile" -color Green
        Update-JsonTimestamp -JsonPath $TEMPJSON -FilePath $_
    } else {
        Log-Write -Msg "Restoring cached object for $($_) from $cacheFile; source=$($tsInfo.ActualTime.ToString('o')), stored=$($tsInfo.StoredTimestamp), delta=$(Format-TimestampDiff -Delta $tsInfo.Delta) sec" -color Green
        Copy-Item -Destination $o -Path $cacheFile -Force
        Log-Write -Msg "Copied cache to build object: $o" -color Cyan
    }
}

# build files
New-Item -Path $MAPFILE -ItemType File -Force

# gcc link
$args_ = @(
    # '-L', "$(Join-Path (Get-Location) 'compile\toolchain\prebuild\i686-elf\lib\gcc\i686-elf\15.2.0\')", 
    '-l', 'gcc',
    "-o", $ST2BIN,
    "-Wl,--strip-all",
    "-nostdlib",
	"-Wl,-T,$($LINKERSCRIPT)", 
	"-Wl,-Map,$($MAPFILE)"
)

Log-Write -Msg "$($GCC) $($args_ -join ' ') $($BOOT2OFILES -join ' ')" -color Blue
$GLINK_OUT = & (Join-Path (Get-Location) 'compile\toolchain\prebuild\gcc.ps1') -Params @($args_, $BOOT2OFILES)
# $GLINK_OUT = & $GCC $args_ $BOOT2OFILES 2>&1
Log-Write -Msg "$($GLINK_OUT -join "`n")"
# $OBJCOPYOUT = objcopy '-O' 'binary' $ST2BIN $ST2BIN 2>&1
# Log-Write -Msg "$($OBJCOPYOUT -join "`n")"


# #wcc link
# $LNK = "
# FORMAT RAW BIN
# OPTION QUIET,
# 		NODEFAULTLIBS,
# 		START=_start,
# 		VERBOSE,
# 		OFFSET=0x8200,
# 		STACK=0x200
# ORDER
# 	CLNAME CODE
# 		SEGMENT _ENTRY
# 		SEGMENT TEXT
# 	CLNAME DATA
# "
# # file $($BOOTA_OBJ)
# # $LNK | Out-File -FilePath (Join-Path $Objdir 'wlink.lnk') -Encoding ASCII -Force
# [System.IO.File]::WriteAllLines((Join-Path $Objdir 'wlink.lnk'), $LNK, [System.Text.Encoding]::ASCII)

# # New-Item -Path (Join-Path $Objdir 'wlink.lnk') -ItemType File -Force
# # Add-Content -Path (Join-Path $Objdir 'wlink.lnk') -Value $LNK

# # call wlink
# # $args_ = @("@$(Join-Path $Objdir 'wlink.lnk')")
# $args_ = @("NAME" , "$($ST2BIN)")
# $BOOT2OFILES|ForEach-Object{if($_){$args_ += @("FILE", $_)}}
# $args_ += @("OPTION", "MAP=$(Join-Path $Build "wlink.map")", "@$(Join-Path $Objdir 'wlink.lnk')")

# Log-Write -Msg "`n`nwlink $($args_ -join ' ')`n`n" -Color Yellow
# $WLINK_OUT = (& $WLINK @args_) 2>&1
# $WLINK_OUT|ForEach-Object{
# 	if($_ -like "*error*"){Log-Write -Msg $_ -color Red}
# 	else{Log-Write -Msg $_ -color Yellow}
# }

if ($LASTEXITCODE -ne 0) {
	Log-Write -Msg "GCC link failed" -Color Red
	throw "Linking failed..."
}
Log-Write -Msg "Linked: $($ST2BIN)" -Color Green

# Img-Push -data (Get-Content -Path $ST2BIN -Raw -Encoding Byte)
