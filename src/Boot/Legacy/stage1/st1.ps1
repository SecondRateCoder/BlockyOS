param(
    [string]$Date,
    [string]$NASM,
    [string]$GCC
)

$Build = Join-Path (Get-Location) ("Build\Build-" + $Date)
# $Image = Join-Path $Build ("floppy-" + $Date + ".img")
$Objdir = Join-Path $Build "\objs\"
$Log = Join-Path $Build ("logst1.txt")
# $ST1OBJ = Join-Path -Path $Objdir "boot1.o"
$ST1BIN = Join-Path -Path $Objdir "boot1.bin"
# $LINKERSCRIPT = Join-Path (Get-Location) "src/Boot/stage1/boot.ld"
# $MAPFILE = Join-Path -Path $Build "gcc_boot1.map"
$FILE = Join-Path -Path (Get-Location) "src\boot\Legacy\stage1\boot.asm"

function Log-Write{
    param(
        [string]$Msg,
        [System.ConsoleColor]$color
    )
    $clean = ""
    $esc=[char]27
    if($color){
        Write-Host $Msg -ForegroundColor $color
        $clean = $Msg -replace "$esc(?:\[[0-9;?]*[ -/]*[@-~]|][^\a]*\a|P.*?$esc\\|X.*?$esc\\|\^.*?$esc\\|_.*?$esc\\|[@-Z\\-_])",""
    }else{
        Write-Host $Msg
        $clean = $Msg
    }
    if(-not (Test-Path $Log)){New-Item $Log -ItemType File}
    Add-Content -Path $Log -Value $clean
}

# function Img-Push{
#     param([byte[]]$data)
#     $file = [System.IO.File]::Open($Image, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
#     $padding = $data.Length
#     do{
#         $padding = [math]::Abs($padding - 512)
#     }while($padding -ge 512)
#     Log-Write -color Yellow -Msg ("Byte array of size: $($data.Length) padded with size: $($padding), starting at address: $((Get-Item -Path $Image).Length) and ending at address: $((Get-Item -Path $Image).Length + $padding + $data.Length)")
#     try{
#         if($data){$file.Write($data, 0, $data.Length)}
#         if($padding -gt 0){
#             $padBytes = New-Object byte[] $padding
#             $file.Write($padBytes, 0, $padding)
#             Log-Write -color Yellow "Padded $($Image) with $($padding) bytes."
#         }
#     }finally{$file.Close()}
# }

# $LINKERSCRIPT = Join-Path -Path (Get-Location) "src\boot\stage1\boot.ld"
# $OUTOBJ = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($FILE))1.o"
# $OUTBIN = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($FILE))1.bin"
# $MAPFILE = Join-Path $Build "stage1.map"

Log-Write -Msg "$($NASM) -f bin $($FILE) -o $($ST1BIN)" -color Blue

$NASMOUT = (& $NASM "-f" "bin" $FILE "-o" $ST1BIN) 2>&1
$NASMOUT|ForEach-Object{
    if($_ -match 'error'){Log-Write -Msg $_ -color Red}
    else{Log-Write -Msg $_ -color Blue}
}

# Img-Push -data (Get-Content -Path $ST1BIN -Raw -Encoding Byte)