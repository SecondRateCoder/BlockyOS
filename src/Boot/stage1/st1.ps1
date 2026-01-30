param(
    [string]$Date,
    [string]$NASM,
    [string]$GCC
)

$Build = Join-Path (Get-Location) ("Build\Build-" + $Date)
$Image = Join-Path $Build ("floppy-" + $Date + ".img")
$Objdir = Join-Path $Build "\objs\"
$Log = Join-Path $Build ("logst1.txt")

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
            Log-Write -color Yellow "Padded $($Image) with $($padding) bytes."
        }
    }finally{$file.Close()}
}

$FILE = Join-Path -Path (Get-Location) "src\boot\stage1\boot.asm"
# $LINKERSCRIPT = Join-Path -Path (Get-Location) "src\boot\stage1\boot.ld"
# $OUTOBJ = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($FILE))1.o"
$OUTBIN = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($FILE))1.bin"
# $MAPFILE = Join-Path $Build "stage1.map"

Log-Write -Msg "$($NASM) -f obj $($FILE) -o $($OUTBIN)" -color Yellow

$NASMOUT = (& $NASM "-f" "bin" $FILE "-o" $OUTBIN) 2>&1
$NASMOUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

# $ARGS = @("-o", $OUTBIN,
# 	"-z", "nostartfiles",
# 	"-ffreestanding", "-fdiagnostics-color=always",
# 	"-T", "$($LINKERSCRIPT)", 
# 	"-Map", "$($MAPFILE)"
# )

# $GCCOUT = (& $GCC -Image "ld" -Params @($OUTOBJ, "-o", $OUTBIN, $ARGS))
# $GCCOUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

Img-Push -data (Get-Content -Path $OUTBIN -Raw -Encoding Byte)