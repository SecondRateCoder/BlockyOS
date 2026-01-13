param(
	[string]$Date,
	[string]$NASM,
	[string]$WCC,
	[string]$WLINK
)

# wcc -mm -zc -s -fo=main.obj main.c
# wlink format raw name program.bin file main.obj

$Build = Join-Path (Get-Location) ("Build/Build-" + $Date)
$Image = Join-Path $Build ("floppy-$($Date).img")
$Objdir = Join-Path $Build "objs"
$Log = Join-Path $Build ("log_.txt")
$Log_st1 = Join-Path $Build ("log_st1.txt")

$BOOTA = (Join-Path (Get-Location) "src/Boot/stage2/boot.asm")
$BOOTC = (Join-Path (Get-Location) "src/Boot/stage2/boot.c")
$BOOTA_OBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT))_a2.obj"
$BOOTC_OBJ = Join-Path -Path $Objdir -ChildPath "$([System.IO.Path]::GetFileNameWithoutExtension($BOOTC))_c2.obj"
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
	"-fo=$($BOOTC_OBJ -replace "\\","/")",
	"-fr=$($Log -replace "\\","/")", 
	"-i=$((Join-Path (Get-Location) 'src/Boot/stage2/') -replace "\\","/")",
	"-i=$((Join-Path (Get-Location) 'src/') -replace "\\","/")",
	"-mm", 
	"-zastd=c99",
	# "-ef", 
	# "-zl", 
	# "-zld",
	"-zls",
	"-s",
	"$($BOOTC -replace "\\","/")"
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
$WCC_OUT = & $WCC $Command
Log-Write -Msg "WCC: $($WCC_OUT)" -Color Yellow

$NASM_OUT = & $NASM "-f" "obj" $BOOTA "-o" $BOOTA_OBJ
Log-Write -Msg "NASM: $($NASM_OUT)" -Color Yellow

# build args safely
New-Item -Path $ST2_BIN -ItemType File -Force

# Generate .lnk file
$LNK = "
FORMAT RAW BIN
OPTION QUIET,
		NODEFAULTLIBS,
		START=start_,
		VERBOSE,
		OFFSET=0x8200,
		STACK=0X200
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
	"NAME" , "$($ST2_BIN)", "FILE", "$($BOOTA_OBJ)", "FILE", "$($BOOTC_OBJ)", 
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