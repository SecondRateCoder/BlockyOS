param(
	[string]$Date,
	[string]$NASM,
	[string]$GCC,
	[string]$WLINK
)

# wcc -mm -zc -s -fo=main.obj main.c
# wlink format raw name program.bin file main.obj

$Build = Join-Path (Get-Location) ("Build/Build-" + $Date)
$Image = Join-Path $Build ("floppy-$($Date).img")
$Objdir = Join-Path $Build "objs"
$Log = Join-Path $Build ("logst2.txt")
$ST2BIN = Join-Path -Path $Objdir "boot2.bin"
$MAPFILE = Join-Path -Path $Build "gcc_boot2.map"

$LINKERSCRIPT = Join-Path (Get-Location) "/src/Boot/stage2/boot.ld"

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

function Log-Write {
    param(
        [string]$Msg,
        [System.ConsoleColor]$color
    )

    Write-Host $Msg -ForegroundColor $color
    $esc=[char]27
    $clean = $Msg -replace "$esc(?:\[[0-9;?]*[ -/]*[@-~]|][^\a]*\a|P.*?$esc\\|X.*?$esc\\|\^.*?$esc\\|_.*?$esc\\|[@-Z\\-_])",""

    Add-Content -Path $Log -Value $clean
}


function Img-Push {
	param([byte[]]$data)
	$file = [System.IO.File]::Open($Image, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
	$padding = $data.Length
	do{
		$padding = [math]::Abs($padding - 512)
	}while($padding -ge 512)
	Log-Write -color Yellow -Msg (
		"Byte array of size: $($data.Length),
        padded with size: $($padding), 
        starting at address: $((Get-Item -Path $Image).Length) 
        and ending at address: $((Get-Item -Path $Image).Length + $padding + $data.Length)"
    )
	try{
		if($data){$file.Write($data, 0, $data.Length)}
		if($padding -gt 0){
			$padBytes = New-Object byte[] $padding
			$file.Write($padBytes, 0, $padding)
			Log-Write -color Yellow -Msg "Padded $($Image) with $($padding) bytes."
		}
	}finally{$file.Close()}
}

function Get-FolderPriority {
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
	"-D", "LOCALSTANDARDFILE", "-D", "LOCALFILE"
)

# Order files
Log-Write -Msg "Boot2:" -color Blue
(Get-ChildItem -Path (Join-Path (Get-Location) "src/boot/stage2/") -Include @("*.c", "*.asm") -Recurse -Force -File)|ForEach-Object{
    Log-Write -Msg " $($_)," -color Blue
    $COMBINED += $_.FullName
}
Log-Write -Msg "lib32:" -color Blue
(Get-ChildItem -Path (Join-Path (Get-Location) "src/kernel/lib32/") -Include @("*.c", "*.asm") -Recurse -Force -File)|ForEach-Object{
	if($_.BaseName -notin $EXCLUDE){
		Log-Write -Msg " $($_)," -color Blue
		$COMBINED += $_.FullName
	}
}

Log-Write -Msg "Sorted:" -color Blue
(Sort-ByForceOrder -Files $COMBINED -ForceOrder $ForceOrder)|ForEach-Object{
    if($_ -match '.asm'){
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

# Compile all 32-bit files
$cc = 0
$BOOT2CFILES|ForEach-Object{
	Log-Write -Msg "$($GCC) -c $($_) -o $(Join-Path -Path $Objdir "c.$((Get-Item -Path $_).BaseName).o") $($COMPILECLI -join ' ')" -color Blue
	$COMPILEOUT = (& $GCC -Params @("-c", $_, "-o", (Join-Path -Path $Objdir "c.$((Get-Item -Path $_).BaseName).o"), $COMPILECLI)) 2>&1
	$COMPILEOUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}
	$cc++
}

# Compile .asm files
$BOOT2ASMFILES|ForEach-Object{
	$args_ = @("-f", "elf32", $_, "-o", $(Join-Path -Path $Objdir "asm.$((Get-Item -Path $_).BaseName).o"))
	Log-Write -Msg "$($NASM) $($args_ -join ' ')" -color Blue
	$COMPILEOUT = (& $NASM $args_) 2>&1
	$COMPILEOUT|ForEach-Object{
		if($_ -match 'error'){Log-Write -Msg $_ -color Red}
		else{Log-Write -Msg $_ -color Yellow}
	}
	$cc++
}

# build files
New-Item -Path $ST2BIN -ItemType File -Force
New-Item -Path $MAPFILE -ItemType File -Force

# gcc link
$args_ = @()
$BOOT2OFILES = ($BOOT2OFILES | Sort-Object -Unique)
$BOOT2OFILES|ForEach-Object{$args_ += $_}
$LGCC = (Get-ChildItem -Path (Get-Item $GCC).Parent -Filter 'libgcc.a' -Recurse -File)
@(
    "--strip-all",
    "-o", $ST2BIN,
	"-T", "$($LINKERSCRIPT)", 
	"-Map", "$($MAPFILE)", "-m", "elf_i386",
    "-L", "$($LGCC.Directory.FullName)",
    "-l", "gcc"
)|ForEach-Object{$args_ += $_}

Log-Write -Msg "$($GCC) $($args_ -join ' ')" -color Blue
$GLINK_OUT = (& $GCC -Image "ld" -Params $args_) 2>&1 | Out-String
$GLINK_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

# objcopy -O binary $ST2ELF $ST2BIN

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

Img-Push -data (Get-Content -Path $ST2BIN -Raw -Encoding Byte)
