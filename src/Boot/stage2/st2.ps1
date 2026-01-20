param(
	[string]$Date,
	[string]$NASM,
	[string]$WCC,
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
$BOOT16A_OBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT16A))_a2.obj"

$BOOT32A = (Join-Path (Get-Location) "src/Boot/stage2/boot32.asm")
$BOOT32A_OBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT32A))_a2.obj"

$BOOT16C = (Join-Path (Get-Location) "src/Boot/stage2/boot16.c")
$BOOT16C_OBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT16C))_c2.obj"

$BOOT32C = (Join-Path (Get-Location) "src/Boot/stage2/boot32.c")
$BOOT32C_OBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT32C))_c2.obj"

$STRINGC = (Join-Path (Get-Location) "src/kernel/public/public/memory/string.c")
$STRINGC_OBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($STRINGC))_c2.obj"

$MEMC = (Join-Path (Get-Location) "src/kernel/public/public/memory/memory.c")
$MEMC_OBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($MEMC))_c2.obj"

$ST2_BIN = Join-Path -Path $Objdir "boot2.bin"

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

$Command = @(
	"-fr=$($Log -replace "\\","/")", 
	"-i=$((Join-Path (Get-Location) 'src/kernel/public/public/memory/') -replace "\\","/")"
	"-i=$((Join-Path (Get-Location) 'src/Boot/stage2/') -replace "\\","/")",
	"-i=$((Join-Path (Get-Location) 'src/') -replace "\\","/")",
	"-mm", 
	"-zastd=c99",
	"-ef", 
	"-zl", 
	"-zld",
	"-zls",
	"-s"
	# "-zu"
)
# $Command = @(
# 	"-mm", 
# 	"-zastd=c99", 
# 	"-ef", 
# 	"-zl", 
# 	"-zld",
# 	"-fr=$($Log)", 
# 	"-zls", 
# 	"-s", 
# 	"-fo=$($BOOTC_OBJ)",
# 	"-i=$(Join-Path (Get-Location) 'src\Boot\stage2')",
# 	"-i=$(Join-Path (Get-Location) 'src')",
# 	"$($BOOTC)"
# )

Start-Transcript -Path $Log_st1 -Append
$WCC = "C:\WATCOM\binnt64\wcc.exe"
Log-Write -Msg (
	"$($WCC) $($Command -join " ")"
) -Color Yellow
# Compile all 16-bit files
$WCC_OUT = & $WCC "-fo=$($BOOT16C_OBJ -replace "\\","/")" $Command "$($BOOT16C -replace "\\","/")"
Log-Write -Msg "WCC: $($WCC_OUT -join '`n')" -Color Yellow

$WCC_OUT = & $WCC "-fo=$($STRINGC_OBJ -replace "\\","/")" $Command "$($STRINGC -replace "\\","/")"
Log-Write -Msg "WCC: $($WCC_OUT -join '`n')" -Color Yellow

$WCC_OUT = & $WCC "-fo=$($MEMC_OBJ -replace "\\","/")" $Command "$($MEMC -replace "\\","/")"
Log-Write -Msg "WCC: $($WCC_OUT -join '`n')" -Color Yellow

# Compile 32-bit files
$GCC_OUT = & $GCC "-c" $BOOT32C "-o" $BOOT32C_OBJ
Log-Write -Msg "WCC: $($GCC_OUT -join '`n')" -Color Yellow

# Compile 16-bit .asm
$NASM_OUT = & $NASM "-f" "obj" $BOOT16A "-o" $BOOT16A_OBJ
Log-Write -Msg "NASM: $($NASM_OUT)" -Color Yellow
# Compile 16-bit .asm
$NASM_OUT = & $NASM "-f" "obj" $BOOT32A "-o" $BOOT32A_OBJ
Log-Write -Msg "NASM: $($NASM_OUT)" -Color Yellow

# build args safely
New-Item -Path $ST2_BIN -ItemType File -Force

# Generate .lnk file
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
$args_ = @(
	"NAME" , "$($ST2_BIN)", 
	"FILE", "$($BOOT16A_OBJ)", "FILE", "$($BOOT32A_OBJ)",
	"FILE", "$($BOOT16C_OBJ)", "FILE", "$($BOOT32C_OBJ)", 
	"FILE", "$($STRINGC_OBJ)", "FILE", "$($MEMC_OBJ)", 
	"OPTION", "MAP=$(Join-Path $Build "wlink.map")", "@$(Join-Path $Objdir 'wlink.lnk')"
)
Log-Write -Msg "`n`nwlink $($args_ -join ' ')`n`n" -Color Yellow
$WLINK_OUT = & $WLINK @args_ #2>&1
Log-Write -Msg "`n`n" -Color Yellow
foreach($item in $WLINK_OUT){Log-Write -Msg "`n`n$($item)" -Color Yellow}

if ($LASTEXITCODE -ne 0) {
	Log-Write -Msg "wlink failed" -Color Red
	throw "Linking failed..."
}
Log-Write -Msg "Linked: $($ST2_BIN)" -Color Green

Img-Push -data (Get-Content -Path $ST2_BIN -Raw -Encoding Byte)

Stop-Transcript