param(
    [Parameter(Mandatory=$true)]
    [string]$PREFIX,
    [Parameter(Mandatory=$true)]
    [string]$NASM
)

$Build = Join-Path (Get-Location) ("Build\Build-" + $PREFIX)
$Objdir = Join-Path $Build "\objs\"
$Log = Join-Path $Build ("st1.log")
$ST1BIN = Join-Path -Path $Objdir "boot1.bin"
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
    $success = $false
    do{
        $success = $true
        try{
            if(-not (Test-Path $Log)){New-Item $Log -ItemType File}
            Add-Content -Path $Log -Value $clean
        }catch{$success = $true}
    }while($success -eq $false)
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
Log-Write "$($NASMOUT -join "`n")"

# Img-Push -data (Get-Content -Path $ST1BIN -Raw -Encoding Byte)