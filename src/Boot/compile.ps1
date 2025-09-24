param(
    [Parameter(Mandatory=$true)]
    [string[]]$AsmFiles, # List of .asm file paths

    [Parameter(Mandatory=$false)]
    [string]$LinkerScript, # Linker script file path

    [Parameter(Mandatory=$false)]
    [bool]$Run,

    [Parameter(Mandatory = $false)]
    [bool]$Run_Bochs
)
#C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\compile.ps1 -AsmFiles "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot1.asm", "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot2.asm" -Run 1
$NASM = "C:\Users\olusa\AppData\Local\bin\NASM\nasm.exe"
$LD = "ld"
$QEMU = "C:\msys64\ucrt64\bin\qemu-system-x86_64.exe"
$BOCHS = "C:\Users\olusa\Bochs-3.0\bochs.exe"

$Date = (Get-Date -Format "yyyy-MM-dd-ss")
$Build = Join-Path (Get-Location) ("Build\Build-" + $Date)
$Image = Join-Path $Build ("floppy-" + $Date + ".img")
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
    if (-not (Test-Path $Objdir)) {
        New-Item -Path $Objdir -ItemType Directory -Force
    }
    if (-not (Test-Path $Log)) {
        New-Item -Path $Log -ItemType File -Force
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
            $floppy_temp = [System.IO.Path]::GetFileNameWithoutExtension($Image)
            Add-Content -Path $BOCHSRC -Value ($Line_Up + $Image + $Line_Down + $BOCHSLOG)
            # $file.Write($floppy_temp, 0, $floppy_temp.Length)
            # $file.Write($Line_Down, 0, $Line_Down.Length)
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


if($Run){
    Log-Write "Command:  $($QEMU) -fda $($Image)"
    $args_qemu = @("-fda", "$($Image)")
    & $QEMU @args_qemu
}

if($Run_Bochs -and $BOCHSRC){
    & $BOCHS "-f" $BOCHSRC "-debugger" "-q"
}

return $true

# # Check if NASM is available
# if (-not (Get-Command $NASM -ErrorAction SilentlyContinue)) {
#     Write-Error "NASM not found in PATH. Please install NASM."
#     exit 1
# }
# # Check if LD is available
# if (-not (Get-Command LD -ErrorAction SilentlyContinue)) {
#     Write-Error "LD (GNU linker) not found in PATH. Please install binutils or compatible LD."
#     exit 1
# }

# # Directory to store object files
# $objDir = Join-Path (Get-Location) "Build\objs"
# if (-Not (Test-Path $objDir)) {
#     New-Item -ItemType Directory -Path $objDir | Out-Null
# }

# # Assemble each.asm file to an object file
# $objFiles = @()
# foreach ($asm in $AsmFiles) {
#     if (-Not (Test-Path $asm)) {
#         Write-Error "Assembly file not found: $asm"
#         exit 1
#     }
    
#     # Generate object filename (replace.asm with.o)
#     $objFile = Join-Path $objDir ([IO.Path]::GetFileNameWithoutExtension($asm) + ".o")
    
#     # NASM command: Use -f elf64 for 64-bit (modify if targeting 32-bit)
#     # You can adjust -f elf32 if targeting 32-bit
#     $nasmCmd = "nasm -f elf64 `"$asm`" -o `"$objFile`""
    
#     Write-Host "Assembling $asm..."
#     Invoke-Expression $nasmCmd
    
#     if (-not (Test-Path $objFile)) {
#         Write-Error "Failed to assemble $asm"
#         exit 1
#     }
    
#     $objFiles += $objFile
# }

# # Check if linker script exists
# $LinkerScript_Out = "C:\\Users\\olusa\\OneDrive\\Documents\\GitHub\\BlockyOS\\src\\Boot\\boot_linker.ld"
# if (-Not (Test-Path $LinkerScript)) {
#     Write-Error "Linker script not found: $($LinkerScript)"
# }else{
#     $LinkerScript_Out = $LinkerScript
# }

# # Link the object files using ld and the specified linker script
# # This assumes your linker script handles symbols like img_push correctly.
# # If you want to ensure certain functions like img_push are included,
# # you can add linker flags or scripts accordingly.
# $ldCmd = "ld -o `"$OutputFile`" -T `"$LinkerScript`" " + ($objFiles -join " ")

# Write-Host "Linking objects..."
# Invoke-Expression $ldCmd

# if (-not (Test-Path $OutputFile)) {
#     Write-Error "Linking failed: Output file $OutputFile not created"
#     exit 1
# }

# Write-Host "Build successful: $OutputFile"