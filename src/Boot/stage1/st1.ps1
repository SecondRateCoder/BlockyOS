param(
    [string]$Date,
    [string]$NASM
)

$Build = Join-Path (Get-Location) ("Build\Build-" + $Date)
$Image = Join-Path $Build ("floppy-" + $Date + ".img")
$Objdir = Join-Path $Build "\objs\"
$Log = Join-Path $Build ("log-" + $Date + ".txt")

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
$OUT = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($BOOT))1.bin"
$NASM_OUT = & $NASM "-f" "bin" $BOOT "-o" $OUT
Log-Write -Msg "NASM: $($NASM_OUT)" -Color Yellow
Img-Push -data (Get-Content -Path $OUT -Raw -Encoding Byte)