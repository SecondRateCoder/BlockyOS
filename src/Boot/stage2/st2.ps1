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

$EXCLUDE = @()

$BOOT2CFILES = @(
	(Join-Path (Get-Location) "src/Boot/stage2/boot32.c"),
	(Join-Path (Get-Location) "src/kernel/public/public/memory/string.c"),
	(Join-Path (Get-Location) "src/kernel/public/public/memory/memory.c")
)
$BOOT2ASMFILES = @((Join-Path (Get-Location) "src/Boot/stage2/boot.asm"), Get-ChildItem -Path (Join-Path (Get-Location) "src/kernel/lib32/") -Name "*.c" -Recusre -Force)
$BOOT2OFILES = @(
	Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT2ASMFILES[0])).o",
	Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT2CFILES[0])).o",
	Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT2CFILES[1])).o",
	Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT2CFILES[2])).o",
)

# $ST2ELF = Join-Path -Path $Objdir "boot2.elf"
$ST2BIN = Join-Path -Path $Objdir "boot2.bin"

$MAPFILE = Join-Path -Path $Build "gcc.map"

function Log-Write{
	param(
		[string]$Msg,
		[System.ConsoleColor]$color
		)
	Write-Host $Msg -ForegroundColor $color
	Add-Content -Path $Log -Value $Msg
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
	"-nostdlib", "-ffreestanding", "-m32", 
	"-fdiagnostics-color=always", 
	"-I", "$(Join-Path (Get-Location) "src/")", 
	"-I", "$(Join-Path (Get-Location) "src/kernel/lib32/")",
	"-std=c99"
)

# Get all lib32 C files.
$BOOT2CFILES += (Get-ChildItem -Path (Join-Path (Get-Location) "src/kernel/lib32/") -Name "*.c" -Recurse -Force
# Compile all 32-bit files
$BOOT2CFILES|ForEach-Object{
	if($_ -notin $EXCLUDE){
		Log-Write -Msg "$GCC -c $($_) -o $([System.IO.Path]::GetFileNameWithoutExtension($_)).o $($COMPILECLI -join ' ')" -color Yellow
		$BOOT2OFILES += (Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($_).o"),
		$COMPILEOUT = (& $GCC "-c" $_ "-o" (Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($_)).o" $COMPILECLI) 2>&1
		$COMPILEOUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}
	}
}

# Compile .asm files
$BOOT2ASMFILES|ForEach-Object{
	if($_ -notin $EXCLUDE){
		$args_ = @("-f", "obj", $_, "-o", "$([System.IO.Path]::GetFileNameWithoutExtension($_)).o")
		Log-Write -Msg "$($NASM ($args_ -join ' '))" -color Yellow
		$BOOT2OFILES += (Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($_)).o"),
		$COMPILEOUT = (& $NASM $args_) 2>&1
		$COMPILEOUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}
	}
}

# build files
New-Item -Path $ST2BIN -ItemType File -Force
New-Item -Path $MAPFILE -ItemType File -Force

# gcc link
# $args_ = @(
# 	$BOOT32COBJ, $STRINGCOBJ, $MEMCOBJ, 
# 	"-o", $ST2BIN,
# 	"-nodefaultlibs", "-nostartfiles", "-ffreestanding",
# 	"-fdiagnostics-color=always",
# 	"-T", "$(Join-Path (Get-Location) "/src/Boot/stage2/boot.ld")", 
# 	"-Wl,-Map,$($MAPFILE),-m,elf_i386"
# )
# Log-Write -Msg "`n$($GCC) $($args_ -join ' ')`n" -Color White
# $GLINK_OUT = (& $GCC @args_) 2>&1
# $GLINK_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}
# objcopy -O binary $ST2ELF $ST2BIN

#wcc link
$LNK = "
FORMAT RAW BIN
OPTION QUIET,
		NODEFAULTLIBS,
		START=_start,
		VERBOSE,
		OFFSET=0x8200,
		STACK=0x200
ORDER
	CLNAME CODE
		SEGMENT _ENTRY
		SEGMENT TEXT
	CLNAME DATA
"
# file $($BOOTA_OBJ)
# $LNK | Out-File -FilePath (Join-Path $Objdir 'wlink.lnk') -Encoding ASCII -Force
[System.IO.File]::WriteAllLines((Join-Path $Objdir 'wlink.lnk'), $LNK, [System.Text.Encoding]::ASCII)

# New-Item -Path (Join-Path $Objdir 'wlink.lnk') -ItemType File -Force
# Add-Content -Path (Join-Path $Objdir 'wlink.lnk') -Value $LNK

# call wlink
# $args_ = @("@$(Join-Path $Objdir 'wlink.lnk')")
$args_ = @("NAME" , "$($ST2BIN)")
$BOOT2OFILES|ForEach-Object{if($_){$args_ += @("FILE", $_)}}
$args_ += @("OPTION", "MAP=$(Join-Path $Build "wlink.map")", "@$(Join-Path $Objdir 'wlink.lnk')")

Log-Write -Msg "`n`nwlink $($args_ -join ' ')`n`n" -Color Yellow
$WLINK_OUT = (& $WLINK @args_) 2>&1
$WLINK_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

if ($LASTEXITCODE -ne 0) {
	Log-Write -Msg "GCC link failed" -Color Red
	throw "Linking failed..."
}
Log-Write -Msg "Linked: $($ST2BIN)" -Color Green

Img-Push -data (Get-Content -Path $ST2BIN -Raw -Encoding Byte)
