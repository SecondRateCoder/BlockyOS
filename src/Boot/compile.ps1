param(
    [Parameter(Mandatory=$true)]
    [string[]]$AsmFiles,   # List of .asm file paths

    [Parameter(Mandatory=$false)]
    [string]$LinkerScript, # Linker script file path

    # Using the unique ID "\x[NUMBER]" you can append empty sectors of specified NUMBER into the floppy
    [Parameter(Mandatory=$false)]
    [string[]]$ExtraFiles, # Extra Files appended to the floppy image; after the kernel is appended.
    
    [Parameter(Mandatory=$false)]
    [int]$SectorNum,	   # Minimum number of sectors for the floppy image
    
    [Parameter(Mandatory=$false)]
    [bool]$BroadImage,	   # should a copy of the kernel image be made in ".\Build`temp\floppy.img"

    [switch]$Run,		   # Should the kernel be run? with QEMU
    [switch]$Run_Bochs	   # Should the kernel be run? with Bochs
    )
#C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\compile.ps1 -AsmFiles "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot1.asm", "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot2.asm" -Run 1
$NASM = "C:\Users\olusa\AppData\Local\bin\NASM\nasm.exe"
$LD = "ld"
$QEMU = "C:\msys64\ucrt64\bin\qemu-system-x86_64.exe"
$BOCHS = "C:\Users\olusa\Bochs-3.0\bochs.exe"

$Date = (Get-Date -Format "yyyy-MM-dd-ss")
$Build = Join-Path (Get-Location) ("Build\Build-" + $Date)
$Image = Join-Path $Build ("floppy-" + $Date + ".img")
$BroadImageFile = Join-Path (Get-Location) "Build\temp\floppy.img"
$Objdir = Join-Path $Build "objs"
$Log = Join-Path $Build ("log-" + $Date + ".txt")

$BOCHSRC = Join-Path $Build ".bochsrc"
$BOCHSLOG = Join-Path $Build "bochs.log"

$Line_Up = "megs: 32
romimage: file=C:\Users\olusa\Bochs-3.0\BIOS-bochs-latest
vgaromimage: file=C:\Users\olusa\Bochs-3.0\VGABIOS-lgpl-latest.bin
boot: floppy
floppya: 1_44="
$Line_Down = ", status=inserted
display_library: win32, options=`"`gui_debug`"`
pci: enabled=1, chipset=i440fx, slot1=cirrus, slot2=ne2k, slot3=usb_ohci
config_interface: win32config
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
    do{
        $padding = [math]::Abs($padding - 512)
        if($padding -gt 512){
            break
        }

    }while($padding -gt 512)
    Log-Write -color Yellow -Msg ("Byte array of size: $($data.Length) padded with size: $($padding), starting at address: $((Get-Item -Path $Image).Length) and ending at address: $((Get-Item -Path $Image).Length + $padding + $data.Length)")
    try {
        if($data){
            $file.Write($data, 0, $data.Length)
        }
        if ($padding -gt 0) {
            $padBytes = New-Object byte[] $padding
            $file.Write($padBytes, 0, $padding)
            Log-Write -color Yellow "Padded $($Image) with $($padding) bytes."
        }
    } finally {
        $file.Close()
    }
}

function Prepare {
    if (-not (Test-Path $Build)) {
        New-Item -Path $Build -ItemType Directory -Force
    }
    if($BroadImage){
        if(-not (Test-Path $BroadImageFile)){
            Remove-Item -Path $BroadImageFile -Force
            New-Item -Path $BroadImageFile -ItemType File -Force
        }
    }
    if (-not (Test-Path $Objdir)) {
        New-Item -Path $Objdir -ItemType Directory -Force
    }
    if (-not (Test-Path $Log)) {
        New-Item -Path $Log -ItemType File -Force
    }
    if(-not (Test-Path $Image)){
        New-Item -Path $Build -ItemType File -Force
    }
    
    if (-not (Test-Path $NASM)) {
        Log-Write -color Red -Msg "NASM not found at $NASM. Please install NASM."
        exit 1
    }
    
    if (-not (Get-Command $LD -ErrorAction SilentlyContinue)) {
        Log-Write -color Red "GNU Linker not found in PATH. Please install GNU Linker."
        exit 1
    }
    if(-not (Test-Path $BOCHSRC)){
        New-Item -Path $BOCHSRC -ItemType File -Force
        try{
            Add-Content -Path $BOCHSRC -Value ($Line_Up + $Image + $Line_Down + $BOCHSLOG)
        }finally{}
    }
}
function Asm-Compile{
    param(
        [string[]]$fPaths,
        [string]$format
    )
    foreach ($file in $fPaths) {
        try {
            if (Test-Path $file) {
                $outputPath = Join-Path $Objdir ([System.IO.Path]::GetFileNameWithoutExtension($file) + ".bin")
                $argument = "-f "+ $format+ " "+ $file+ " -o "+ $outputPath
                Log-Write -color Yellow "Command: $($NASM) $($argument)"
                $nasmOutput = & $NASM "-f" $format $file "-o" $outputPath "-s"
                if ($?) {
                    Log-Write -color Green "Successfully compiled $($file)"
                    if ($nasmOutput) {
                        # $nasmOutput | ForEach-Object { Log-Write "Output: $($_)" }
                        foreach($item in $nasmOutput){
                            Log-Write "Output: $($item)"
                        }
                    }
                } else {
                    Log-Write -color Red "!Compilation of $($file) failed. NASM Code: $($LASTEXITCODE)"
                    if ($nasmOutput) {
                        # $nasmOutput | ForEach-Object { Log-Write "#!: $_" }
                        foreach($item in $nasmOutput){
                            Log-Write -color Red ("! " + $item)
                        }
                    }
                }
            }
        } catch {
            Log-Write -color Red "CRITICAL ERROR: Exception during compilation of $($file):"
            Log-Write -color Red $_.Exception.ToString()
        }
    }
}

# Optional override for linker script
# $LinkerScript_Out = if ($LinkerScript) { $LinkerScript } else { Join-Path (Get-Location) "Boot\boot_linker.ld" }

# Main Logic
(Prepare)
(Asm-Compile -fPaths $AsmFiles -format "bin")

$succ_ = @()
$cc = 0
Log-Write -color Cyan -Msg "Pushing compiled data"
# Push compiled data
foreach ($file in $AsmFiles) {
    $temp = Join-Path $Objdir ([System.IO.Path]::GetFileNameWithoutExtension($file) + ".bin")
    if(Test-Path $temp){
        $data = Get-Content -Path $temp -Raw -Encoding Byte
        Img-Push -data $data
        $succ_ += $true
        $cc++
    }
}

Log-Write -color Cyan -Msg "Validating compilation"
# Validate compilation successes
foreach($success in $succ_){
    if(($success -eq $false)){
        return $false
    }
}

Log-Write -color Cyan -Msg "Pushing extra files"
# 0x00003D
# Add extra files
if($ExtraFiles){
    foreach($path in $ExtraFiles){
        # This adds an absolute number of bytes
        if(($path[0] -eq '\') -and ($path[1] -eq 'x')){
            $num_empty = [int]($path -replace '\D', '') # Removes all non-digit characters
            $data = New-Object byte[] ($num_empty* 512)
            Log-Write -color Yellow -Msg ("Adding $($num_empty) empty sectors of overall size $($num_empty* 512), starting at address: $((Get-Item -Path $Image).Length) and ending at address: $((Get-Item -Path $Image).Length + ($num_empty* 512))")
            Img-Push -data $data
            # This adds bytes for the next item to be at a certain address
        }elseif(($path[0] -eq '\') -and ($path[1] -eq 'a')){
            $num = 0
            if(($path[2] -eq '0') -and ($path[3] -eq 'x')){
                # Using hex address, convert to decimal
                $hex = if($path -match '0x[0-9A-Fa-f]+'){$matches[0]}else{$null}
                $num = if($hex){[Convert]::ToInt32(($hex -replace '^0x', ''), 16)}else{$null}
                if(-not ($hex -and $num)){
                    Log-Write -color Red -Msg "Invalid hex address: `n`tinput: $($path)"
                }
            }else{
                # Using decimal address, use address
                $num = [int]($path -replace '\D', '') # Remove all non-digit characters
            }
            if($num % 512 -ne 0){
                Log-Write -color Red -Msg "Invalid address, nust be a multiple of 512, address was: `n`tDecimal: $($num), `n`t Hexa-decimal: $('{0:X}' -f $num)"
                continue
            }else{
                if($num -lt (Get-Item -Path $Image).Length){
                    Log-Write -color Red -Msg "File exceeded the input address, `n`tFile Size: $((Get-Item -Path $Image).Length), `n`taddress: `n`tDecimal: $($num), `n`tHexa-decimal: 0x$('{0:X}' -f $num)"
                }elseif($num -gt (Get-Item -Path $Image).Length){
                    Log-Write -color Green -Msg "Padding to address: `n`taddress: `n`t`tDecimal: $($num), `n`t`tHexa-decimal: 0x$('{0:X}' -f $num), `n`tFile size: $((Get-Item -Path $Image).Length), `n`tpadding: $($num - (Get-Item -Path $Image).Length)"
                    $data = New-Object byte[] ($num - (Get-Item -Path $Image).Length)
                    Img-Push -data $data
                }
            }
        }elseif(Test-Path -Path $path){
            $data = New-Object byte[] 512
            $data_f = Get-Content -Path $path -Raw -Encoding Byte
            $data_n = [System.Text.Encoding]::Default.GetBytes([System.IO.Path]::GetFileName($path))
            $cc = 0
            foreach($char in $data_n){
                $data[$cc] = $data_n[$cc]
            }
            foreach($byte in $data_f){
                $data[$cc] = $data_f[$cc]
            }
            Img-Push -data $data
        }else{
            Log-Write -color Red -Msg ("File at: " + $path + " is invalid or does not exist");
        }
    }
}

# text.txt at 0x000051FE
# Calculate how much padding is needed
$targetSize = $SectorNum * 512
$padding = [math]::Abs($targetSize - (Get-Item $Image).Length)
Log-Write -color Cyan -Msg "Padding to $($padding) size"
if(-not ($padding -eq (Get-Item $Image).Length)){
    # Pad if needed
    if ($padding -gt 0) {
        $padBytes = New-Object byte[] $padding
        $cc = 0;
        foreach($char in [System.Text.Encoding]::Default.GetBytes(("Default.txt This is an empty text file"))){
            $padBytes[$cc] = $char
        }
        Img-Push -data $padBytes
        Log-Write -color Yellow "Padded $Image with $padding bytes."
    } else {
        Log-Write -color Yellow "$Image is already $((Get-Item $Image).Length) bytes or larger."
    }
}
if($BroadImage){
    Copy-Item -Path $Image -Destination $BroadImageFile
}
if($Run -eq $true){
    Log-Write -color Yellow "Command:  $($QEMU) -fda $($Image)"
    $args_qemu = @("-fda", "$($Image)")
    & $QEMU @args_qemu
}elseif(($Run_Bochs -eq $true) -and $BOCHSRC){
    & $BOCHS "-f" $BOCHSRC "-debugger" "-q"
}


return $true