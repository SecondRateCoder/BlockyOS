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
    [int]$Hidden,
    [switch]$emudebug,
    [Parameter(Mandatory=$false)]
    [string]$EXEdebug,
    [Parameter(Mandatory=$true)]
    [string]$prefix = (Get-Date -Format "yyyy-MM-dd-ss").ToString(),
    [Parameter(Mandatory=$false)]
    [int]$SectorSize = 512
)
$GCC = 'gcc'
# $NASM = ${env:NASM}
$NASM = 'nasm'
$QEMU = if(${env:qemu-x86_64}){${env:qemu-x86_64}}else{'qemu-system-x86_64'}
$BOCHS = if(${env:bochs}){${env:bochs}}else{'bochs'}
$STDLIB = Join-Path (Get-Location) "\src\kernel\lib32\stdkernel\stdkernel.ps1"
$Build = Join-Path (Get-Location) ("Build\Build-" + $prefix)
$Image = Join-Path $Build "disk.img"
$BootPartitionDir = Join-Path $Build 'boot-part/'
$BootPartitionBlob = Join-Path $BootPartitionDir "legacyblob.bin"
$UEFIBootBlob = Join-Path $BootPartitionDir "blob.efi"
$BroadImageFile = Join-Path (Get-Location) "Build\temp\floppy.img"
$Objdir = Join-Path $Build "objs"
$FATOUTDIR = Join-Path $Build 'emu/FAT32/'
$DiskConfigJson = Join-Path (Get-Location) "compile\disk-config.json"
$GPTScript = Join-Path (Get-Location) "compile\emu\GPT.ps1"
$FSScript = Join-Path (Get-Location) "compile\emu\FS.ps1"

$BOCHSRC = Join-Path $Build ".bochsrc"
$BOCHSLOG = Join-Path $Build "bochs.log"
$debuglog = Join-Path $Build "emudebug.log"
$debuggerlog = Join-Path $Build "debugger.log"
$bochsPath = $env:bochs
$bochsDir  = Split-Path $bochsPath

$args_qemu = @(
    "-drive", "if=pflash,format=raw,readonly=on,file=$(Join-Path (Get-Location) 'compile/toolchain/uefi/OVMF_CODE_4M.fd')",
    "-drive", "if=pflash,format=raw,file=$(Join-Path (Get-Location) 'compile/toolchain/uefi/OVMF_VARS_4M.fd')"
    "-drive", "file=$($Image),format=raw,if=ide"
    "-vga", "cirrus", "-full-screen"
    "-cpu", "host", "-m", "1024"
    "-chardev", "file,id=dbg,path=$($debuglog)", 
    "-dump-vmstate", "$(Join-Path $Build "qemudump.json")"
    "-device", "isa-debugcon,iobase=0xE9,chardev=dbg", "-usb",
    "-debugcon", "stdio",
    "-d", "guest_errors,unimp,pcall,io"
)
$BOCHSFILE = "
cpu: model=core2_penryn_t9600, count=1, reset_on_triple_fault=1, ips=10000000
boot: disk
vgaromimage: file=$(Join-Path (Get-Location) 'compile/toolchain/qemu/OVMF_CODE_4M.fd')
romimage: file=$(Get-ChildItem -Path $bochsDir -Recurse -Filter 'BIOS-bochs-latest*' | Select-Object -ExpandProperty FullName)
memory: guest=1024, host=1024
ata0-master: type=disk, path=`"$($Image)`", mode=flat, status=inserted, translation=lba
vga: extension=cirrus, update_freq=60
sound: driver=default
speaker: enabled=1, mode=sound
e1000: enabled=1
usb_ohci: enabled=1
pci: enabled=1, chipset=i440fx, slot1=cirrus, slot2=e1000, slot3=usb_ohci
display_library: win32, options=`"`gui_debug`"
config_interface: win32config
magic_break: enabled=1
port_e9_hack: enabled=1, all_rings=1
debugger_log: $($debuggerlog)
log: $($BOCHSLOG)
iodebug: all_rings=1
"

function UEFI-Populate{
	param(
		[string]$PartitionBlob,
		[string[]]$Partitions,
		[string]$DiskBytes
	)
	Img-Push -data (Get-Content $PartitonBlob)
	foreach($blob in $Partitions){Img-Push -data (Get-Content $blob)}
	Img-Push -data (Get-Content $DiskBytes)
}

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
            if(-not (Test-Path $debuglog)){New-Item $Log -ItemType File}
            Add-Content -Path $debuglog -Value $clean
        }catch{$success = $true}
    }while($success -eq $false)
}


function Img-Push{
    param([byte[]]$data)
    $file = [System.IO.File]::Open($Image, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
    try{
        # Write the data first
        $file.Write($data, 0, $data.Length)
        (Img-Pad)
    }finally{
        if($file){
            try{$file.Close()}catch{}
            try{$file.Dispose()}catch{}
        }
    }
}

function Img-Pad{
    # Ensure no processes are holding the file
    Start-Sleep -Milliseconds 500
    $attemptCount = 0
    while($attemptCount -lt 5){
        try{
            $file = [System.IO.File]::Open($Image, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
            break
        }catch{
            $attemptCount++
            if($attemptCount -ge 5){throw}
            Start-Sleep -Milliseconds 1000
        }
    }
    $currentSize = (Get-Item $Image).Length
    $padding = ($SectorSize - ($currentSize % $SectorSize)) % $SectorSize
    try{
        if($padding -gt 0){
            $padBytes = New-Object byte[] $padding
            $file.Write($padBytes, 0, $padding)
            Log-Write -color Yellow "Padded $($Image) with $($padding) bytes."
        }
    }finally{$file.Close()}
}

function Prepare {
    if($clear -eq $true){
        Remove-Item (Join-Path (Get-Location) "/Build/") -Force -Recurse
        New-Item -Path $Build -ItemType Directory -Force
    }
    if(-not(Test-Path $Build)){New-Item -Path $Build -ItemType Directory -Force}
    if($broadimage){
        if(-not(Test-Path $BroadImageFile)){
            Remove-Item -Path $BroadImageFile -Force -ErrorAction SilentlyContinue
            New-Item -Path $BroadImageFile -ItemType File -Force
        }
        if(Test-Path $BroadImageFile) {Remove-Item -Path $BroadImageFile -Force -ErrorAction SilentlyContinue
        }else{
            New-Item -Path (Split-Path $BroadImageFile -Parent) -ItemType Directory -Force | Out-Null
            New-Item -Path $BroadImageFile -ItemType File -Force | Out-Null
        }
    }
    if(-not(Test-Path $Objdir)){New-Item -Path $Objdir -ItemType Directory -Force}
    if(-not(Test-Path $FATOUTDIR)){New-Item -Path $FATOUTDIR -ItemType Directory -Force}
    if(-not(Test-Path $BootPartitionDir)){New-Item -Path $BootPartitionDir -ItemType Directory -Force}
    if(-not(Test-Path $debuglog)){New-Item -Path $debuglog -ItemType File -Force}
    if(-not(Test-Path $Image)){New-Item -Path $Build -ItemType File -Force -ErrorAction SilentlyContinue}
    Write-Host (& $NASM -v)
    if($LASTEXITCODE -ne 0){
        Log-Write -color Red -Msg "NASM not found at $($NASM). Please install NASM."
        exit 1
    }
    Write-Host (& 'gcc' -v)
    if($LASTEXITCODE -ne 0){
        Log-Write -color Red -Msg "GCC not found at $(GCC). Please install GCC."
        exit 1
    }
    if(-not(Test-Path $BOCHSRC)){
        New-Item -Path $BOCHSRC -ItemType File -Force
        Add-Content -Path $BOCHSRC -Value $BOCHSFILE
    }
    (& $STDLIB)
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
    try{
        if($null -eq $token){throw "Token is null"}
        if($token.StartsWith('\x')){
            $num = Parse-Number $token.Substring(2)
            if($num -lt 0){throw "Negative size not allowed"}
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
                Log-Write -color Red -Msg "Error: file length ($($fileLen)) exceeds target ($($num))."
                return
            }

            $pad = $num - $fileLen
            if($pad -gt 0){
                $data = New-Object byte[] $pad
                Img-Push -data $data
            }else{Log-Write -color Yellow -Msg "No padding needed; file length equals target."}
            return
        }
        Log-Write -color Red -Msg "Token not recognized: $($token)"
    }catch{Log-Write -color Red -Msg "Handle-PadToken error: $($_.Exception.Message)"}
}

function Compile-Watcom{
    param([string]$src, [string[]]$wargs)
    $argList = @()
    $out = Join-Path $Objdir "$([System.IO.Path]::GetFileNameWithoutExtension($src)).bin"
    if($wargs){$argList += $wargs -split '\s+'}
    # produce an output object/exe; adjust flags to your toolchain (wcc/wcl usage may differ)
    $argList += @($src, "-fo=$($out)")
    Log-Write -color Yellow -Msg ("WATCOM: " + $WATCOM + " " + ($argList -join ' '))
    $proc = & $WCC @argList 2>&1
    if($LASTEXITCODE -ne 0){
        Log-Write -color Red -Msg ("WATCOM failed: " + ($proc -join "`n"))
        Img-Push -data (Get-Item -Path $src)
        return $false
    }
    return $true
}


(Prepare)

Log-Write -color Cyan "===== Step 1: Compile Bootloaders ====="
Log-Write -color Cyan "===== Step 1.1: Compile Legacy BootLoader Blob ====="
if(-not (Test-Path $BootPartitionDir)){New-Item -Path $BootPartitionDir -ItemType Directory -Force | Out-Null}
(& "$(Join-Path (Get-Location) 'src/Boot/Legacy/legacy.ps1')" -prefix $prefix -NASM $NASM -GCC $GCC -EMUOUT $BootPartitionDir)
if(-not (Test-Path $BootPartitionBlob)){
    Log-Write -color Red -Msg "Legacy Bootloader blob not created: $BootPartitionBlob"
    exit 1
}
Log-Write -color Green "Legacy bootloader compiled: $BootPartitionBlob"

Log-Write -color Cyan "===== Step 1.2: Compile UEFI BootLoader Blob ====="
(& "$(Join-Path (Get-Location) 'src/Boot/UEFI/UEFI.ps1')" -prefix $prefix -NASM $NASM -GCC $GCC -EMUOUT $BootPartitionDir)
if(-not (Test-Path $UEFIBootBlob)){
    Log-Write -color Red -Msg "UEFI Bootloader blob not created: $UEFIBootBlob"
    exit 1
}
Log-Write -color Green "UEFI bootloader compiled: $UEFIBootBlob"

Log-Write -color Cyan "===== Step 2: Create Disk Layout (GPT) ====="
if(Test-Path $Image){Remove-Item $Image -Force}
if(-not (Test-Path $DiskConfigJson)){
    Log-Write -color Red -Msg "Disk config not found: $DiskConfigJson"
    exit 1
}
Log-Write -color Yellow "Creating GPT disk: $($Image)"
(& $GPTScript -JsonFile $DiskConfigJson -OutputPath $Image -LogFile (Join-Path $Build "gpt.log") -Verbose)

Log-Write -color Cyan "===== Step 3: Combine Boot Partition (Legacy + UEFI) ====="
$legacyBytes = [System.IO.File]::ReadAllBytes($BootPartitionBlob)
$uefiBytes = [System.IO.File]::ReadAllBytes($UEFIBootBlob)
$bootPartSize = 33554432  # 32MB
$combinedSize = $legacyBytes.Length + $uefiBytes.Length
if($combinedSize -gt $bootPartSize){
    Log-Write -color Red -Msg "Combined bootloaders ($combinedSize bytes) exceed boot partition size ($bootPartSize bytes)"
    exit 1
}
$paddingNeeded = $bootPartSize - $combinedSize
Log-Write -color Green "Boot partition: Legacy ($($legacyBytes.Length)) + UEFI ($($uefiBytes.Length)) + Padding ($paddingNeeded) = $bootPartSize bytes"

Log-Write -color Cyan "===== Step 4: Assemble Final Disk ====="
# Combine Legacy + UEFI + Padding into boot bytes
$bootBytes = New-Object byte[] $bootPartSize
[Array]::Copy($legacyBytes, 0, $bootBytes, 0, $legacyBytes.Length)
[Array]::Copy($uefiBytes, 0, $bootBytes, $legacyBytes.Length, $uefiBytes.Length)
# Padding bytes are already zero-initialized
$diskFile = [System.IO.File]::Open($Image, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
try{
    $diskFile.Write($bootBytes, 0, $bootBytes.Length)
    Log-Write -color Green "Boot partition written to disk ($(($bootBytes.Length / 1MB)) MB)."
}finally{
    $diskFile.Close()
    $diskFile.Dispose()
}
Log-Write -color Green "Disk image finalized: $($Image)"

Log-Write -color Cyan "===== Step 5: Format Boot Partition (FAT32) ====="
if(Test-Path $FSScript){
    Log-Write -color Yellow "Formatting boot partition with FAT32..."
    try{
        (& $FSScript -FileSystemType 'FAT32' -PartitionImage $Image -PartitionOffset 1048576 -SourceDirectory $BootPartitionDir -LogFile (Join-Path $Build "fs.log") -Verbose)
        Log-Write -color Green "Boot partition formatted successfully"
    }catch{Log-Write -color Yellow "FS.ps1 formatting encountered an issue: $_"}
}else{Log-Write -color Yellow "FS.ps1 not found at $FSScript, skipping FAT32 formatting"}

if($broadimage){Copy-Item -Path $Image -Destination $BroadImageFile}
if($run){
    Log-Write -color Yellow -Msg "Command:  $($QEMU) $($args_qemu -join ' ')"
    $QEMUOUT = ""
    if($emudebug -and $EXEdebug -and (Test-Path $EXEdebug)){
        $QEMUOUT = & $QEMU @args_qemu "-S" "-gdb" "tcp::1234"
        $GDBOUT = & gdb $EXEdebug
        Log-Write "$($GDBOUT -join "`n")"
        $GDBOUT = & target remote localhost:1234
        Log-Write "$($GDBOUT -join "`n")"
    }else{$QEMUOUT = & $QEMU @args_qemu}
    Log-Write "$($QEMUOUT -join "`n")"
}elseif($runbochs){
    $env:Path += $Build
    Copy-Item -Path (Get-ChildItem -Path (Get-Location) -Name -Filter "bx_enh_dbg.ini") -Destination (Join-Path $Build "\bx_enh_dbg.ini")
    Log-Write -Msg "$($BOCHS) -f $($BOCHSRC) -dbg -q -dbglog $($debuggerlog)
    `n$(Get-Content $BOCHSRC)" -color Blue
    & $BOCHS "-f" $BOCHSRC "-debugger" "-q" "-dbglog" $($debuggerlog)
    Copy-Item -Path (Join-Path $Build "\bx_enh_dbg.ini") -Destination (Get-ChildItem -Path (Get-Location) -Name -Filter "bx_enh_dbg.ini")
}

