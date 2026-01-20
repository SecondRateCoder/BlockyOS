# ./Compiles/Run.ps1 -clear -BroadImage -SectorNum 2880 -Reserved 0 -Hidden 0

param(
    [string[]]$Extrafiles,
    # [Parameter(Mandatory=$false)]
    [switch]$runbochs,
    # [Parameter(Mandatory=$false)]
    [switch]$run,
    # [Parameter(Mandatory=$false)]
    [switch]$clear,
    [switch]$broadimage,
    [Parameter(Mandatory=$false)]
    [int]$SectorNum,
    [Parameter(Mandatory=$false)]
    [int]$Reserved,
    [Parameter(Mandatory=$false)]
    [int]$Hidden
)

$NASM = "C:\Users\olusa\AppData\Local\bin\NASM\nasm.exe"
$LD = "ld"
$QEMU = "C:\msys64\ucrt64\bin\qemu-system-x86_64.exe"
$BOCHS = "C:\Users\olusa\Bochs-3.0\bochs.exe"
$WCC = "wcc"
$WLINK = "wlink"
# $ST1 = Join-Path (Get-Location) "Boot\stage1\st1.ps1"
$ST1 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\stage1\st1.ps1"
# $ST2 = Join-Path (Get-Location) "Boot\stage2\st2.ps1"
$ST2 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\stage2\st2.ps1"

$Date = (Get-Date -Format "yyyy-MM-dd-ss")
$Build = Join-Path (Get-Location) ("Build\Build-" + $Date)
$Image = Join-Path $Build ("floppy-" + $Date + ".img")
$BroadImageFile = Join-Path (Get-Location) "Build\temp\floppy.img"
$Objdir = Join-Path $Build "objs"
$Log = Join-Path $Build ("log-" + $Date + ".txt")

$BOCHSRC = Join-Path $Build ".bochsrc"
$BOCHSLOG = Join-Path $Build "bochs.log"

$LINEUP = "megs: 32
romimage: file=C:\Users\olusa\Bochs-3.0\BIOS-bochs-latest
vgaromimage: file=C:\Users\olusa\Bochs-3.0\VGABIOS-lgpl-latest.bin
boot: floppy
floppya: 1_44="
$LINEDWN = ", status=inserted
display_library: win32, options=`"`gui_debug`"`
pci: enabled=1, chipset=i440fx, slot1=cirrus, slot2=ne2k, slot3=usb_ohci
config_interface: win32config
vga: extension=vbe
magic_break: enabled=1
log: "

function Log-Write {
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
    do{$padding = [math]::Abs($padding - 512)}while($padding -ge 512)
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

function Prepare {
    if(-not(Test-Path $Build)){New-Item -Path $Build -ItemType Directory -Force}
    if($broadimage){
        if(-not(Test-Path $BroadImageFile)){
            Remove-Item -Path $BroadImageFile -Force
            New-Item -Path $BroadImageFile -ItemType File -Force
        }
        if(Test-Path $BroadImageFile) {Remove-Item -Path $BroadImageFile -Force -ErrorAction SilentlyContinue
        }else{
            New-Item -Path (Split-Path $BroadImageFile -Parent) -ItemType Directory -Force | Out-Null
            New-Item -Path $BroadImageFile -ItemType File -Force | Out-Null
        }
    }
    if(-not(Test-Path $Objdir)){New-Item -Path $Objdir -ItemType Directory -Force}
    if(-not(Test-Path $Log)){New-Item -Path $Log -ItemType File -Force}
    if(-not(Test-Path $Image)){New-Item -Path $Build -ItemType File -Force}
    
    if(-not(Test-Path $NASM)){
        Log-Write -color Red -Msg "NASM not found at $NASM. Please install NASM."
        exit 1
    }
    
    if(-not(Get-Command $LD -ErrorAction SilentlyContinue)){
        Log-Write -color Red "GNU Linker not found in PATH. Please install GNU Linker."
        exit 1
    }
    if(-not(Test-Path $BOCHSRC)){
        New-Item -Path $BOCHSRC -ItemType File -Force
        try{
            Add-Content -Path $BOCHSRC -Value ($LINEUP + $Image + $LINEDWN + $BOCHSLOG)
        }finally{}
    }
}

function Parse-Number {
    param([string]$s)
    $s = $s.Trim()
    if ($s -match '^0x([0-9A-Fa-f]+)$') { return [int64]::Parse($matches[1], [System.Globalization.NumberStyles]::HexNumber) }
    if ($s -match '^[0-9]+$') { return [int64]$s }
    if ($s -match '0x([0-9A-Fa-f]+)') { return [int64]::Parse($matches[1], [System.Globalization.NumberStyles]::HexNumber) }
    if ($s -match '([0-9]+)') { return [int64]$matches[1] }
    throw "Unable to parse number from '$s'"
}

function Handle-PadToken {
    param(
        [string]$token
        # [string]$filePath  # required for \a
    )

    try {
        if($null -eq $token){throw "Token is null"}

        if($token.StartsWith('\x')){
            $num = Parse-Number $token.Substring(2)
            if ($num -lt 0) { throw "Negative size not allowed" }
            $data = New-Object byte[] $num
            Img-Push -data $data
            return
        }

        if($token.StartsWith('\a')){
            # if (-not $filePath) { Log-Write -color Red -Msg "Missing file path for `\a token"; return }
            # if (-not (Test-Path $filePath)) { Log-Write -color Red -Msg "File not found: $filePath"; return }

            $num = Parse-Number $token.Substring(2)
            if($num -lt 0){throw "Negative address not allowed"}

            $fileLen = (Get-Item $file).Length
            if($fileLen -gt $num){
                Log-Write -color Red -Msg "Error: file length ($fileLen) exceeds target ($num)."
                return
            }

            $pad = $num - $fileLen
            if($pad -gt 0){
                $data = New-Object byte[] $pad
                Img-Push -data $data
            }else{Log-Write -color Yellow -Msg "No padding needed; file length equals target."}
            return
        }
        Log-Write -color Red -Msg "Token not recognized: $token"
    }catch{Log-Write -color Red -Msg "Handle-PadToken error: $($_.Exception.Message)"}
}

function Compile-Watcom{
    param([string]$src, [string[]]$wargs)
    $argList = @()
    $out = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($src)).bin"
    if ($wargs) { $argList += $wargs -split '\s+' }
    # produce an output object/exe; adjust flags to your toolchain (wcc/wcl usage may differ)
    $argList += @($src, "-fo=$($out)")
    Log-Write -color Yellow -Msg ("WATCOM: " + $WATCOM + " " + ($argList -join ' '))
    $proc = & $WCC @argList 2>&1
    if ($LASTEXITCODE -ne 0) {
        Log-Write -color Red -Msg ("WATCOM failed: " + ($proc -join "`n"))
        Img-Push -data (Get-Item -Path $src)
        return $false
    }
    return $true
}

if($clear -eq $true){Remove-Item (Join-Path (Get-Location) "Build") -Force -Recurse}
(Prepare)
(. $ST1 -Date $Date -NASM $NASM)
(. $ST2 -Date $Date -NASM $NASM -WCC $WCC -WLINK $WLINK -GCC $GCC)

$cc = 0;
Handle-PadToken -token "\x$($Reserved.ToString())"
Handle-PadToken -token "\x$($Hidden.ToString())"
foreach($item in $ExtraFiles){
    if($item[0] -eq '\'){
        switch($item[1]){
            'w'{Compile-Watcom -src $item[1] -wargs $ExtraFiles[$cc + 1]}
            'a'{Handle-PadToken -token $item[1]}
            'x'{Handle-PadToken -token $item[1]}
        }
    }
    $cc++
}

$targetSize = $SectorNum * 512
$padding = [math]::Abs($targetSize - (Get-Item $Image).Length)
Log-Write -color Cyan -Msg "Padding to $($padding) size"
if(-not ($padding -eq (Get-Item $Image).Length)){
    # Pad if needed
    if ($padding -gt 0) {
        $padBytes = New-Object byte[] $padding
        $cc = 0;
        Img-Push -data $padBytes
        Log-Write -color Yellow "Padded $Image with $padding bytes."
    } else {
        Log-Write -color Yellow "$Image is already $((Get-Item $Image).Length) bytes or larger."
    }
}

if($broadimage){Copy-Item -Path $Image -Destination $BroadImageFile}
if($run){
    Log-Write -color Yellow "Command:  $($QEMU) -fda $($Image)"
    $args_qemu = @("-fda", "$($Image)",  "-vga", "std")
    & $QEMU @args_qemu
}elseif($runbochs){
    & $BOCHS "-f" $BOCHSRC "-debugger" "-q"
}

