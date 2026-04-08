param(
	[Parameter(Mandatory=$true)]
	[string]$prefix,
	[Parameter(Mandatory=$true)]
	[string]$NASM,
	[Parameter(Mandatory=$true)]
    [string]$GCC,
	[Parameter(Mandatory=$true)]
    [string]$EMUOUT
)

$Build = Join-Path (Get-Location) ("Build/Build-" + $prefix)
$Objdir = Join-Path $Build "\objs\"
$ST1BIN = Join-Path -Path $Objdir "boot1.bin"
$ST2BIN = Join-Path -Path $Objdir "boot2.bin"
$Image = Join-Path -Path $EMUOUT "legacyblob.bin"
$SRC = Join-Path (Get-Location) '/src/Boot/'
$Log = Join-Path $Build 'legacy.log'

function Log-Write{
    param(
        [string]$Msg,
        [System.ConsoleColor]$color
    )
    # $clean = $Msg -replace "$esc(?:\[[0-9;?]*[ -/]*[@-~]|][^\a]*\a|P.*?$esc\\|X.*?$esc\\|\^.*?$esc\\|_.*?$esc\\|[@-Z\\-_])",""
    $esc=[char]27
    $clean = $Msg -replace "$($esc)\[[0-9;]*[A-Za-z]",""
    if($color){Write-Host $Msg -ForegroundColor $color
    }else{Write-Host $Msg}
    $success = $false
    do{
        $success = $true
        try{
            if(-not (Test-Path $Log)){New-Item $Log -ItemType File}
            Add-Content -Path $Log -Value $clean
        }catch{$success = $true}
    }while($success -eq $false)
}

function Img-Push {
    param([byte[]]$data)
    $file = [System.IO.File]::Open($Image, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
    $padding = $data.Length
    do{$padding = [math]::Abs($padding - 256)
    }while($padding -ge 256)
    Log-Write -color Yellow -Msg ("Byte array of size: $($data.Length) padded with size: $($padding), starting at address: $((Get-Item -Path $Image).Length) and ending at address: $((Get-Item -Path $Image).Length + $padding + $data.Length)")
    try {
        if($data){$file.Write($data, 0, $data.Length)}
        if($padding -gt 0){
            $padBytes = New-Object byte[] $padding
            $file.Write($padBytes, 0, $padding)
            Log-Write -color Yellow "Padded $($Image) with $($padding) bytes."
        }
    }finally{$file.Close()}
}

(& "$($SRC)/Legacy/stage1/st1.ps1" -prefix $prefix -NASM $NASM)
(& "$($SRC)/Legacy/stage2/st2.ps1" -prefix $prefix -NASM $NASM -GCC $GCC)

Img-Push -data ([System.IO.File]::ReadAllBytes($ST1BIN))
Img-Push -data ([System.IO.File]::ReadAllBytes($ST2BIN))