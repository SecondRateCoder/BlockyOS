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
$ST2ELF = Join-Path -Path $Objdir "boot2.elf"
$ST2BIN = Join-Path -Path $Objdir "boot2.bin"
$MAPFILE = Join-Path -Path $Build "gcc.map"

$LINKERSCRIPT = Join-Path (Get-Location) "/src/Boot/stage2/boot.ld"

$EXCLUDE = @(
	"stdfile", "f-rat", "stdprogram"
)

$BOOT2CFILES = @()
$BOOT2ASMFILES = @()
$BOOT2OFILES = @()

New-Item -Path $Log -ItemType File -Force

(Get-ChildItem -Path (Join-Path (Get-Location) "src/boot/stage2/") -Filter "*.c" -Recurse -Force -File)|ForEach-Object{
	$BOOT2CFILES += $_.FullName
	$BOOT2OFILES += Join-Path $Objdir "$($_.BaseName).o"
}
(Get-ChildItem -Path (Join-Path (Get-Location) "src/kernel/lib32/") -Filter "*.c" -Recurse -Force -File)|ForEach-Object{
	if($_.BaseName -notin $EXCLUDE){
		Log-Write -Msg $_ -color Blue
		$BOOT2CFILES += $_.FullName
		$BOOT2OFILES += Join-Path $Objdir "$($_.BaseName).o"
	}
}

(Get-ChildItem -Path (Join-Path (Get-Location) "src/boot/stage2/") -Filter "*.asm" -Recurse -Force -File)|ForEach-Object{
	$BOOT2ASMFILES += $_.FullName
	$BOOT2OFILES += Join-Path $Objdir "$($_.BaseName).o"
}
(Get-ChildItem -Path (Join-Path (Get-Location) "src/kernel/lib32/") -Filter "*.asm" -Recurse -Force -File)|ForEach-Object{
	if($_.BaseName -notin $EXCLUDE){
		Log-Write -Msg $_ -color Blue
		$BOOT2ASMFILES += $_.FullName
		$BOOT2OFILES += Join-Path $Objdir "$($_.BaseName).o"
	}
}

function Log-Write{
	param(
		[string]$Msg,
		[System.ConsoleColor]$color
		)
	Write-Host $Msg -ForegroundColor $color
	Add-Content -Path $Log -Value ($Msg -replace "`e\[[0-9;]*[A-Za-z]", "")
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
			starting at address: $((Get-Item -Path $Image).Length) and 
			ending at address: $((Get-Item -Path $Image).Length + $padding + $data.Length)")
	try{
		if($data){$file.Write($data, 0, $data.Length)}
		if($padding -gt 0){
			$padBytes = New-Object byte[] $padding
			$file.Write($padBytes, 0, $padding)
			Log-Write -color Yellow -Msg "Padded $($Image) with $($padding) bytes."
		}
	}finally{$file.Close()}
}

$COMPILECLI = @(
	"-nostdlib", "-m32", 
	"-fdiagnostics-color=always",  "-fno-leading-underscore", "-ffreestanding",
	"-I", "$(Join-Path (Get-Location) "src/")", 
	# "-I", "$(Join-Path (Get-Location) "src/kernel/lib32/")",
	"-std=c99"
)

# Compile all 32-bit files
$cc = 0
$BOOT2CFILES|ForEach-Object{
	Log-Write -Msg "$($GCC) -c $($_) -o $($BOOT2OFILES[$cc]) $($COMPILECLI -join ' ')" -color Yellow
	$COMPILEOUT = (& $GCC -Params @("-c", $_, "-o", $BOOT2OFILES[$cc], $COMPILECLI)) 2>&1
	$COMPILEOUT|ForEach-Object{
		if($_ -like "*error*"){Log-Write -Msg $_ -color Red}
		else{Log-Write -Msg $_ -color Yellow}
	}
	$cc++
}

# Compile .asm files
$BOOT2ASMFILES|ForEach-Object{
	$args_ = @("-f", "elf32", $_, "-o", "$($BOOT2OFILES[$cc])")
	Log-Write -Msg "$($NASM) $($args_ -join ' ')" -color Yellow
	$COMPILEOUT = (& $NASM $args_) 2>&1
	$COMPILEOUT|ForEach-Object{
		if($_ -like "*error*"){Log-Write -Msg $_ -color Red}
		else{Log-Write -Msg $_ -color Yellow}
	}
	$cc++
}

# build files
New-Item -Path $ST2BIN -ItemType File -Force
New-Item -Path $MAPFILE -ItemType File -Force

# gcc link
$ST2ELF = Join-Path -Path $Objdir "boot2.elf"
$args_ = @()
$BOOT2OFILES|ForEach-Object{$args_ += $_}
@("-o", $ST2ELF,
	"-z", "nostartfiles",
	"-ffreestanding", "-fdiagnostics-color=always",
	"-T", "$($LINKERSCRIPT)", 
	"-Map", "$($MAPFILE)", "-m", "elf_i386"
)|ForEach-Object{$args_ += $_}

Log-Write -Msg "`n$($GCC) $($args_ -join ' ')`n" -Color White

$GLINK_OUT = (& $GCC -Image "ld" -Params $args_) 2>&1
$GLINK_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

$ST2GCCBIN = Join-Path -Path $Objdir "boot2gcc.bin"
objcopy -O binary $ST2ELF $ST2GCCBIN

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
