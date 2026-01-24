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

$BOOT = Join-Path -Path (Get-Location) "src\Boot\stage1\boot.asm"
$OUTOBJ = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT))1.o"
$OUTBIN = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT))1.bin"
# Log-Write -Msg "$($NASM) -f elf $($BOOT) -o $($OUTOBJ)" -color White
# $NASMOUT = (& $NASM "-f" "elf" $BOOT "-o" $OUTOBJ) 2>&1
# $NASMOUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}
# objdump.exe -a $OUTOBJ
# $arg_ = @(
#     $OUTOBJ, 
#     "-o", $OUTBIN,
#     "-nodefaultlibs", "-nostartfiles",
#     "-fdiagnostics-color=always",
#     "-T", "$(Join-Path (Get-Location) "/src/Boot/stage1/boot.ld")"
# )
# Log-Write -Msg "$($GCC) $($arg_ -join ' ')" -color White
# $GCCOUT = (& $GCC $arg_) 2>&1
# $GCCOUT|ForEach-Object{Log-Write $_ -color Yellow}
Log-Write -Msg "$($NASM) -f bin $($BOOT) -o $($OUTBIN)" -color White
$NASMOUT = (& $NASM "-f" "bin" $BOOT "-o" $OUTBIN) 2>&1
$NASMOUT|ForEach-Object{Log-Write -Msg $_ -color Yellow}

Img-Push -data (Get-Content -Path $OUTBIN -Raw -Encoding Byte)