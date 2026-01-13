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
$num_empty = 0

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
panic: action=ask
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
    }while($padding -ge 512)
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

$WATCOM = "wcc"  # adjust path to your Watcom compiler
$WLINK  = "wlink" # optional for linking

function Compile-Watcom {
    param($src, $args, $out)
    $argList = @()
    if ($args) { $argList += $args -split '\s+' }
    # produce an output object/exe; adjust flags to your toolchain (wcc/wcl usage may differ)
    $argList += @($src, "-fo=$out")
    Log-Write -color Yellow -Msg ("WATCOM: " + $WATCOM + " " + ($argList -join ' '))
    $proc = & $WATCOM @argList 2>&1
    if ($LASTEXITCODE -ne 0) {
        Log-Write -color Red -Msg ("WATCOM failed: " + ($proc -join "`n"))
        return $false
    }
    return $true
}

function Asm-Compile{
    param(
        [string[]]$fPaths,
        [string]$format
    )
    $cc = 0;
    foreach ($file in $fPaths) {
        try {
            if (-not $file) { $cc++; continue }
            # NASM .asm compile path
            if (((Test-Path $file) -eq $true) -and (($file[0] -ne '\') -and ($file[1] -ne 'x'))) {
                $outputPath = Join-Path $Objdir ([System.IO.Path]::GetFileNameWithoutExtension($file) + ".bin")
                $argument = "-f "+ $format+ " "+ $file+ " -o "+ $outputPath
                Log-Write -color Yellow "Command: $($NASM) $($argument)"
                $nasmOutput = & $NASM "-f" $format $file "-o" $outputPath "-s"
                if ($?) {
                    Log-Write -color Green "Successfully compiled $($file)"
                    if ($nasmOutput) {
                        foreach($item in $nasmOutput){
                            Log-Write -color Yellow -Msg "Output: $($item)"
                        }
                    }
                    # push compiled binary immediately
                    if (Test-Path -Path $outputPath) {
                        $data = Get-Content -Path $outputPath -Raw -Encoding Byte
                        Img-Push -data $data
                    }
                } else {
                    Log-Write -color Red "!Compilation of $($file) failed. NASM Code: $($LASTEXITCODE)"
                    if ($nasmOutput) {
                        foreach($item in $nasmOutput){
                            Log-Write -color Red ("! " + $item)
                        }
                    }
                }

            # \x - append N empty sectors (count is digits in token)
            } elseif(($file[0] -eq '\') -and ($file[1] -eq 'x')){
                $num_empty = [int]($file -replace '\D', '') # Removes all non-digit characters
                $data = New-Object byte[] ($num_empty* 512)
                Log-Write -color Yellow -Msg ("Adding $($num_empty) empty sectors of overall size $($num_empty* 512), starting at address: $((Get-Item -Path $Image).Length) and ending at address: $((Get-Item -Path $Image).Length + ($num_empty* 512))")
                Img-Push -data $data

            # \a - pad to absolute address (hex or dec)
            } elseif(($file[0] -eq '\') -and ($file[1] -eq 'a')){
                $num = 0
                if($file.Length -ge 4 -and ($file[2] -eq '0') -and ($file[3] -eq 'x')){
                    $hex = if($file -match '0x[0-9A-Fa-f]+'){$matches[0]}else{$null}
                    $num = if($hex){[Convert]::ToInt32(($hex -replace '^0x', ''), 16)}else{$null}
                    if(-not ($hex -and $num)){
                        Log-Write -color Red -Msg "Invalid hex address: `n`tinput: $($file)"
                        $cc++; continue
                    }
                }else{
                    $num = [int]($file -replace '\D', '') # Remove all non-digit characters
                }
                if($num % 512 -ne 0){
                    Log-Write -color Red -Msg "Invalid address, must be a multiple of 512, address was: `n`tDecimal: $($num), `n`tHexa-decimal: $('{0:X}' -f $num)"
                    $cc++; continue
                }
                $curr = (Get-Item -Path $Image).Length
                if($num -lt $curr){
                    Log-Write -color Red -Msg "File exceeded the input address, `n`tFile Size: $curr, `n`taddress: `n`tDecimal: $($num), `n`tHexa-decimal: 0x$('{0:X}' -f $num)"
                } elseif($num -gt $curr){
                    $pad = $num - $curr
                    Log-Write -color Green -Msg "Padding to address: `n`taddress: `n`t`tDecimal: $($num), `n`t`tHexa-decimal: 0x$('{0:X}' -f $num), `n`tFile size: $curr, `n`tpadding: $pad"
                    $data = New-Object byte[] ($pad)
                    Img-Push -data $data
                } # else equal, nothing to do

            } else {
                # If path exists, handle according to extension.
                if (Test-Path -Path $file) {
                    $ext = [System.IO.Path]::GetExtension($file).TrimStart('.').ToLower()
                    switch ($ext) {
                        'asm' {
                            # if .asm passed here treat same as above (in case - was escaped)
                            $outputPath = Join-Path $Objdir ([System.IO.Path]::GetFileNameWithoutExtension($file) + ".bin")
                            Log-Write -color Yellow "Command: $($NASM) -f $format $file -o $outputPath"
                            $nasmOutput = & $NASM "-f" $format $file "-o" $outputPath "-s"
                            if ($?) {
                                Log-Write -color Green "Successfully compiled $($file)"
                                $data = Get-Content -Path $outputPath -Raw -Encoding Byte
                                Img-Push -data $data
                            } else {
                                Log-Write -color Red "!Compilation of $($file) failed. NASM Code: $($LASTEXITCODE)"
                            }
                        }
                        'c' {
                            $outElf = Join-Path $Objdir ([System.IO.Path]::GetFileNameWithoutExtension($file) + ".elf")
                            $outBin = Join-Path $Objdir ([System.IO.Path]::GetFileNameWithoutExtension($file) + ".bin")

                            # Produce an object/ELF. Use -nostdlib/-ffreestanding if building freestanding code.
                            Log-Write -color Yellow "GCC: $GCC $file -o $outElf -nostdlib"
                            $gccOut = & $GCC $file "-o" $outElf 2>&1 "-nostdlib"
                            if ($LASTEXITCODE -ne 0) {
                                Log-Write -color Red "GCC failed for $($file): $($gccOut)"
                                break
                            }

                            # Convert ELF/COFF to flat binary
                            if (-not (Get-Command objcopy -ErrorAction SilentlyContinue)) {
                                Log-Write -color Red "objcopy not found in PATH; cannot produce flat binary."
                            } else {
                                Log-Write -color Yellow "objcopy -O binary $outElf $outBin"
                                $objcopyOut = & objcopy -O binary $outElf $outBin 2>&1
                                if ($LASTEXITCODE -ne 0) {
                                    Log-Write -color Red "objcopy failed: $objcopyOut"
                                } else {
                                    Log-Write -color Green "GCC produced flat binary $outBin"
                                    $data = Get-Content -Path $outBin -Raw -Encoding Byte
                                    Img-Push -data $data
                                }
                            }
                        }
                        default {
                            # For ExtraFiles that are real files, include metadata header + file contents (preserve previous behavior)
                            $fi = Get-Item -Path $file
                            $data_f = Get-Content -Path $file -Raw -Encoding Byte
                            Log-Write -color Yellow -Msg "Adding file $($file) to floppy image,`n`tFloppy Size: $((Get-Item -Path $Image).Length)"
                            # Build FAT-like metadata header (same as previous implementation)
                            $ba = @([System.Text.Encoding]::ASCII.GetBytes($fi.Name.PadRight(11).Substring(0,11))) #Name
                            $ba +=  @([byte]0) + # attributes
                                    @([byte]0) + # _reserved
                                    @([byte]0) + # creationtime_tenths or hundredths
                                    [BitConverter]::GetBytes([uint16]($fi.CreationTime.ToFileTime() -shr 16 -band 0xFFFF)) + # creationtime
                                    [BitConverter]::GetBytes([uint16]($fi.CreationTime.ToFileTime() -shr 32 -band 0xFFFF)) + # creationdate
                                    [BitConverter]::GetBytes([uint16]($fi.LastAccessTime.ToFileTime() -shr 32 -band 0xFFFF)) + # access_date
                                    [BitConverter]::GetBytes([uint16]0) + # fst_clusterhigh
                                    [BitConverter]::GetBytes([uint16]($fi.LastWriteTime.ToFileTime() -shr 16 -band 0xFFFF)) + # modifiedtime
                                    [BitConverter]::GetBytes([uint16]($fi.LastWriteTime.ToFileTime() -shr 32 -band 0xFFFF)) + # modified_date
                                    [BitConverter]::GetBytes([uint16]0) + # fst_clusterlow
                                    [BitConverter]::GetBytes([uint32]($fi.Length)) #Size

                            $max = if($ba.Length -gt $data_f.Length){$ba.Length}else{$data_f.Length}
                            $data = New-Object byte[] ($fi.Length + $ba.Length)
                            for($i = 0; $i -lt $max; $i++){
                                if($i -lt $ba.Length){$data[$i] = $ba[$i]}
                                if($i -lt $data_f.Length){$data[$i + $ba.Length] = $data_f[$i]}
                            }
                            Img-Push -data $data
                        }
                    }
                }elseif(Test-Path -Path ($file.Substring(2))){
                    if($file[0] -eq '\' -and $file[0] -eq 'w'){
                        Compile-Watcom -src $file.Substring(2) -args "format dos", "-zc", "-s", "-ms" -out ($Objdir + "$([System.IO.Path]::GetFileNameWithoutExtension($file.Substring(2))).bin")
                    }
                }else{
                    Log-Write -color Red -Msg ("File at: $($file) is invalid or does not exist");
                }
            }
        } catch {
            Log-Write -color Red -Msg "CRITICAL ERROR: Exception during processing of $($file):"
            Log-Write -color Red -Msg $_.Exception.ToString()
        }
        $cc++
    }

    # Now process ExtraFiles (if any) using the same logic as above.
    if ($ExtraFiles) {
        foreach ($path in $ExtraFiles) {
            try {
                # reuse the same token handling as above by invoking this function recursively would be awkward;
                # Instead replicate the specific extra-file behaviors: \x (empty sectors), \a (absolute pad) and file add.
                if(($path[0] -eq '\') -and ($path[1] -eq 'x')){
                    $num_empty = [int]($path -replace '\D', '')
                    $data = New-Object byte[] ($num_empty* 512)
                    Log-Write -color Yellow -Msg ("Adding $($num_empty) empty sectors of overall size $($num_empty* 512)")
                    Img-Push -data $data
                } elseif(($path[0] -eq '\') -and ($path[1] -eq 'a')) {
                    # absolute pad (same handling as above)
                    $num = 0
                    if($path.Length -ge 4 -and ($path[2] -eq '0') -and ($path[3] -eq 'x')){
                        $hex = if($path -match '0x[0-9A-Fa-f]+'){$matches[0]}else{$null}
                        $num = if($hex){[Convert]::ToInt32(($hex -replace '^0x', ''), 16)}else{$null}
                    } else {
                        $num = [int]($path -replace '\D', '')
                    }
                    if($num % 512 -ne 0){
                        Log-Write -color Red -Msg "Invalid address, must be multiple of 512: $path"; continue
                    }
                    $curr = (Get-Item -Path $Image).Length
                    if($num -gt $curr){
                        $pad = $num - $curr
                        Img-Push -data (New-Object byte[] ($pad))
                    }
                } elseif (Test-Path -Path $path) {
                    # create metadata + file push (replicate previous ExtraFiles block)
                    $fi = Get-Item -Path $path
                    $data_f = Get-Content -Path $path -Raw -Encoding Byte
                    $ba = @([System.Text.Encoding]::ASCII.GetBytes($fi.Name.PadRight(11).Substring(0,11)))
                    $ba +=  @([byte]0) + @([byte]0) + @([byte]0) +
                        [BitConverter]::GetBytes([uint16]($fi.CreationTime.ToFileTime() -shr 16 -band 0xFFFF)) +
                        [BitConverter]::GetBytes([uint16]($fi.CreationTime.ToFileTime() -shr 32 -band 0xFFFF)) +
                        [BitConverter]::GetBytes([uint16]($fi.LastAccessTime.ToFileTime() -shr 32 -band 0xFFFF)) +
                        [BitConverter]::GetBytes([uint16]0) +
                        [BitConverter]::GetBytes([uint16]($fi.LastWriteTime.ToFileTime() -shr 16 -band 0xFFFF)) +
                        [BitConverter]::GetBytes([uint16]($fi.LastWriteTime.ToFileTime() -shr 32 -band 0xFFFF)) +
                        [BitConverter]::GetBytes([uint16]0) +
                        [BitConverter]::GetBytes([uint32]($fi.Length))

                    $max = if($ba.Length -gt $data_f.Length){$ba.Length}else{$data_f.Length}
                    $data = New-Object byte[] ($fi.Length + $ba.Length)
                    for($i = 0; $i -lt $max; $i++){
                        if($i -lt $ba.Length){$data[$i] = $ba[$i]}
                        if($i -lt $data_f.Length){$data[$i + $ba.Length] = $data_f[$i]}
                    }
                    Img-Push -data $data
                } else {
                    Log-Write -color Red -Msg ("ExtraFile at: $($path) is invalid or does not exist");
                }
            } catch {
                Log-Write -color Red -Msg "CRITICAL ERROR: Exception during ExtraFiles processing: $path"
                Log-Write -color Red -Msg $_.Exception.ToString()
            }
        }
    }
}

# ...existing code...

# // filepath: c:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\Compiles\compile.ps1
# ...existing code...

# Replace the separate "Pushing compiled data" loop and the standalone ExtraFiles handling
# with a single call — Asm-Compile now handles compiled outputs and ExtraFiles.


# Main Logic
(Prepare)
(Asm-Compile -fPaths $AsmFiles -format "bin")

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
            $cc += 1
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