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
$Log = Join-Path $Build ("logST2.txt")

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

$COMPILECLI = @(
	"-nostdlib", "-I", "$(Join-Path (Get-Location) "src/")",
	"-fdiagnostics-color=always", "-m32",
	"-std=c99"
)

# Compile all 32-bit files
# boot32.c
Log-Write -Msg "`n$($GCC) -c $($BOOT32C) -o $($BOOT32COBJ) $($COMPILECLI -join ' ')`n" -Color White
$GCC_OUT = (& $GCC "-c" $BOOT32C "-o" $BOOT32COBJ $COMPILECLI) 2>&1
$GCC_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}
# string.c
Log-Write -Msg "`n$($GCC) -c $($STRINGC) -o $($STRINGCOBJ) $($COMPILECLI -join ' ')`n" -Color White
$GCC_OUT = (& $GCC "-c" $STRINGC "-o" $STRINGCOBJ $COMPILECLI) 2>&1
$GCC_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}
# mem.c
Log-Write -Msg "`n$($GCC) -c $($MEMC) -o $($MEMCOBJ) $($COMPILECLI -join ' ')`n" -Color White
$GCC_OUT = (& $GCC "-c" $MEMC "-o" $MEMCOBJ $COMPILECLI) 2>&1
$GCC_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

# Compile boot16.asm
Log-Write -Msg "`n$($NASM) -f obj $($BOOT16A) -o $($BOOT16AOBJ)`n" -Color White
$NASM_OUT = (& $NASM "-f" "obj" $BOOT16A "-o" $BOOT16AOBJ) 2>&1# | grc --colour=on
$NASM_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

# build files
New-Item -Path $ST2BIN -ItemType File -Force

# gcc link
$args_ = @(
	$BOOT32COBJ, $STRINGCOBJ, $MEMCOBJ, 
	"-o", $ST2BIN,
	"-nodefaultlibs", "-nostartfiles",
	"-fdiagnostics-color=always",
	"-T", "$(Join-Path (Get-Location) "/src/Boot/stage2/boot.ld")"
)
Log-Write -Msg "`n$($GCC) $($args_ -join ' ')`n" -Color White
$GLINK_OUT = (& $GCC @args_) 2>&1
$GLINK_OUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

if ($LASTEXITCODE -ne 0) {
	Log-Write -Msg "GCC link failed" -Color Red
	throw "Linking failed..."
}
Log-Write -Msg "Linked: $($ST2BIN)" -Color Green

Img-Push -data (Get-Content -Path $ST2BIN -Raw -Encoding Byte)