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
$Log = Join-Path $Build ("log_.txt")
$Log_st1 = Join-Path $Build ("log_st1.txt")

$BOOT16A = (Join-Path (Get-Location) "src/Boot/stage2/boot16.asm")
$BOOT16AOBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT16A)).obj"

$BOOT32C = (Join-Path (Get-Location) "src/Boot/stage2/boot32.c")
$BOOT32COBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT32C)).obj"

$STRINGC = (Join-Path (Get-Location) "src/kernel/public/public/memory/string.c")
$STRINGCOBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($STRINGC)).obj"

$MEMC = (Join-Path (Get-Location) "src/kernel/public/public/memory/memory.c")
$MEMCOBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($MEMC)).obj"

$ST2BIN = Join-Path -Path $Objdir "boot2.bin"

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
	Log-Write -color Yellow -Msg ("Byte array of size: $($data.Length) padded with size: $($padding), starting at address: $((Get-Item -Path $Image).Length) and ending at address: $((Get-Item -Path $Image).Length + $padding + $data.Length)")
	try{
		if($data){$file.Write($data, 0, $data.Length)}
		if($padding -gt 0){
			$padBytes = New-Object byte[] $padding
			$file.Write($padBytes, 0, $padding)
			Log-Write -color Yellow -Msg "Padded $($Image) with $($padding) bytes."
		}
	}finally{$file.Close()}
}

Start-Transcript -Path $Log_st1 -Append

$COMPILECLI = @(
	"-nostdlib", "-i", "$(Join-Path (Get-Location) "src/kernel/")",
	"-fdiagnostics-color=always",
	2<&1
)

# Compile all 32-bit files
# boot32.c
$GCC_OUT = & $GCC "-c" $BOOT32C "-o" $BOOT32COBJ $COMPILECLI
Log-Write -Msg "GCC: $($GCC_OUT -join '`n')" -Color Yellow
# string.c
$GCC_OUT = & $GCC "-c" $STRINGC "-o" $STRINGCOBJ $COMPILECLI
Log-Write -Msg "GCC: $($GCC_OUT -join '`n')" -Color Yellow
# mem.c
$GCC_OUT = & $GCC "-c" $MEMC "-o" $MEMCOBJ $COMPILECLI
Log-Write -Msg "GCC: $($GCC_OUT -join '`n')" -Color Yellow

# Compile  boot16.asm
$NASM_OUT = & $NASM "-f" "obj" $BOOT16A "-o" $BOOT16AOBJ
Log-Write -Msg "NASM: $($NASM_OUT)" -Color Yellow

# build files
New-Item -Path $ST2BIN -ItemType File -Force

# gcc link
$args_ = @(
	"-c" $BOOT32COBJ, "-c" $STRINGCOBJ, "-c" $MEMCOBJ, 
	"-flinker-output=exec", "-nodefaultlibs", "-nostartfiles",
	"-T", "$(Join-Path (Get-Location) "kernel/Boot/stage1/boot.ld")",
	2<&1
)
Log-Write -Msg "`n`nwlink $($args_ -join ' ')`n`n" -Color Yellow
$GLINK_OUT = & $GCC @args_
Log-Write -Msg "$($GLINK_OUT -join "`n")" -Color Yellow

if ($LASTEXITCODE -ne 0) {
	Log-Write -Msg "wlink failed" -Color Red
	throw "Linking failed..."
}
Log-Write -Msg "Linked: $($ST2BIN)" -Color Green

Img-Push -data (Get-Content -Path $ST2BIN -Raw -Encoding Byte)

Stop-Transcript
