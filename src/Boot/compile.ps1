param(
    [Parameter(Mandatory=$true)]
    [string[]]$AsmFiles, # List of .asm file paths

    [Parameter(Mandatory=$false)]
    [string]$LinkerScript, # Linker script file path

    [Parameter(Mandatory=$false)]
    [bool]$Run,
    
    [Parameter(Mandatory = $false)]
    [bool]$Run_Bochs,

    [Parameter(Mandatory=$false)]
    [string[]]$ExtraFiles,

    [switch]$BroadImage,

    [Parameter(Mandatory=$false)]
    [int]$SectorNum
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
    param([string]$Msg)
    Write-Host $Msg
    Add-Content -Path $Log -Value $Msg
}

function Img-Push {
    param([byte[]]$data)
    $file = [System.IO.File]::Open($Image, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
    try {
        if($data){
            $file.Write($data, 0, $data.Length)
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
        Log-Write "NASM not found at $NASM. Please install NASM."
        exit 1
    }
    
    if (-not (Get-Command $LD -ErrorAction SilentlyContinue)) {
        Log-Write "GNU Linker not found in PATH. Please install GNU Linker."
        exit 1
    }
    if(-not (Test-Path $BOCHSRC)){
        New-Item -Path $BOCHSRC -ItemType File -Force
        try{
            Add-Content -Path $BOCHSRC -Value ($Line_Up + $Image + $Line_Down + $BOCHSLOG)
        }finally{
        }
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
                Log-Write "Command: $($NASM) $($argument)"
                $nasmOutput = & $NASM "-f" $format $file "-o" $outputPath "-s"
                if ($?) {
                    Log-Write "Successfully compiled $($file)"
                    if ($nasmOutput) {
                        # $nasmOutput | ForEach-Object { Log-Write "Output: $($_)" }
                        foreach($item in $nasmOutput){
                            Log-Write "Output: $($item)"
                        }
                    }
                } else {
                    Log-Write "!Compilation of $($file) failed. NASM Code: $($LASTEXITCODE)"
                    if ($nasmOutput) {
                        # $nasmOutput | ForEach-Object { Log-Write "#!: $_" }
                        foreach($item in $nasmOutput){
                            Log-Write ("! " + $item)
                        }
                    }
                }
            }
        } catch {
            Log-Write "CRITICAL ERROR: Exception during compilation of $($file):"
            Log-Write $_.Exception.ToString()
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
foreach ($file in $AsmFiles) {
    $temp = Join-Path $Objdir ([System.IO.Path]::GetFileNameWithoutExtension($file) + ".bin")
    if(Test-Path $temp){
        $data = Get-Content -Path $temp -Raw -Encoding Byte
        Img-Push -data $data
        $succ_ += $true
        $cc++
    }
}

foreach($success in $succ_){
    if(($success -eq $false)){
        return $false
    }
}


# Add extra files
if($ExtraFiles){
    foreach($path in $ExtraFiles){
        if(Test-Path -Path $path){
            $data = Get-Content -Path $path -Raw -Encoding Byte
            Img-Push -data $data
        }else{
            Log-Write -Msg ("File at: " + $path + " is invalid or does not exist");
        }
    }
}

# Calculate how much padding is needed
$targetSize = $SectorNum * 512
$padding = $targetSize - (Get-Item $Image).Length

# Pad if needed
if ($padding -gt 0) {
    $padBytes = New-Object byte[] $padding
    Img-Push -data $padBytes
    Log-Write "Padded $Image with $padding bytes."
} else {
    Log-Write "$Image is already $((Get-Item $Image).Length) bytes or larger."
}

if($BroadImage){
    Copy-Item -Path $Image -Destination $BroadImageFile
}

if($Run){
    Log-Write "Command:  $($QEMU) -fda $($Image)"
    $args_qemu = @("-fda", "$($Image)")
    & $QEMU @args_qemu
}

if($Run_Bochs -and $BOCHSRC){
    & $BOCHS "-f" $BOCHSRC "-debugger" "-q"
}

return $true