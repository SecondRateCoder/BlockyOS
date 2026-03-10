param(
    [Parameter(Mandatory=$true)]
    [string]$Date,
    [Parameter(Mandatory=$true)]
    [string]$NASM,
    [Parameter(Mandatory=$true)]
    [string]$GCC
)

$EFIPARENT = "C:/gnu-efi-master/"
$ARCHITECTURE = "x86_64"

$Build = Join-Path (Get-Location) ("Build\Build-" + $Date)
$Log = Join-Path $Build ("UEFI/logUEFI.log")
$Objdir = Join-Path $Build "\objs\UEFI\"
$Image = Join-Path $Build ("floppy-" + $Date + ".img")
$MAPFILE = Join-Path $Build "UEFI.map"
$UEFISHAREDBLOB = (Join-Path -Path $Objdir "/blob.so")
$UEFIBINARYBLOB = (Join-Path -Path $Objdir "/blob.bin")
[hashtable]$UEFIBINARIESTABLE = @{
    'UEFI/boot.efi' = $UEFIBINARYBLOB
}
$OBJDOUTF = Join-Path $Build "UEFI/headerdump.log"

$GCC = 'gcc'

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
    if(-not (Test-Path $Log)){
        New-Item (Join-Path $Build ("UEFI")) -ItemType Directory
        New-Item $Log -ItemType File
    }
    Add-Content -Path $Log -Value $clean
}

$CARGS = @(
    "-I", "$(Get-Location)/src/",
    "-I", "$($EFIPARENT)/inc", "-nostdlib",
    "-fno-stack-protector", "-fno-builtin", "-fno-exceptions", "-fshort-wchar"
    # "$(Get-Location)/compile/toolchain/prebuild/i686-elf/include/",
)

$LARGS = @(
    "-o", $UEFISHAREDBLOB, 
    "-nostdlib", "-shared", "-Bsymbolic", 
    "-Wl,-T,$($EFIPARENT)/gnuefi/elf_$($ARCHITECTURE)_efi.lds", "-Wl,-Map,$($MAPFILE)",
    "$($EFIPARENT)/$($ARCHITECTURE)/lib/libefi.a", "$($EFIPARENT)/gnuefi/crt0-efi-$($ARCHITECTURE).s"
)

$FILES = Get-ChildItem -Path (Join-Path (Get-Location) "src/Boot/UEFI") -Include "*.c" -Recurse |
        Where-Object { $_.FullName -notmatch '\\Partition\\' -and $_.FullName -notmatch '/Partition/' }
$Objects = @()
$FILES | ForEach-Object{
    $f = Get-Item $_
    Log-Write -Msg "$($GCC) -c $_ -o $(Join-Path $Objdir $f.BaseName).o $($CARGS -join ' ')" -color Blue
    $GCCOUT = & $GCC '-c' $_ '-o' "$(Join-Path $Objdir $f.BaseName).o" $CARGS
    Log-Write -Msg ($GCCOUT -join "`n")
    if(Test-Path "$(Join-Path $Objdir $f.BaseName).o"){
        $Objects += "$(Join-Path $Objdir $f.BaseName).o"
    }else{exit 1}
}
Log-Write -Msg "$($GCC) $($LARGS -join ' ') $($Objects -join ' ')" -color Blue
$GCCOUT = & $GCC $LARGS $Objects
Log-Write -Msg "$($GCCOUT -join "`n")"
Log-Write -Msg "$(objcopy.exe) -O pei-x86-64 $($UEFISHAREDBLOB) $($UEFIBINARYBLOB)"
$OBJCOUT = & objcopy.exe '-O' 'pei-x86-64' $UEFISHAREDBLOB $UEFIBINARYBLOB
Log-Write -Msg "$($OBJCOUT -join "`n")"
Add-Content $OBJDOUTF (& objdump.exe '-x' $UEFIBINARYBLOB)
Add-Content (Join-Path $Build 'UEFI/table.json') | ConvertTo-Json $UEFIBINARIESTABLE

$PARTITIONOUT = Join-Path $Objdir 'partition.bin'
$PARTITIONSRC = (Join-Path (Get-Location) "src/Boot/UEFI/Partition")

$PFILES = Get-ChildItem -Path $PARTITIONSRC -Include "*.c" -Recurse
$POBJFILES = @()
$PFILES|ForEach-Object{
    $f = Get-Item $_
    Log-Write -Msg "$($GCC) -nostdlib -fno-stack-protector -fno-builtin -fno-exceptions -c $_ -o $(Join-Path $Objdir $f.BaseName).o" -color Blue
    $GCCOUT = & $GCC '-nostdlib' '-fno-stack-protector' '-fno-builtin' '-fno-exceptions' '-c' $_ '-o' "$(Join-Path $Objdir $f.BaseName).o"
    Log-Write -Msg ($GCCOUT -join "`n")
    if(Test-Path "$(Join-Path $Objdir $f.BaseName).o"){
        $POBJFILES += "$(Join-Path $Objdir $f.BaseName).o"
    }else{exit 1}
}
Log-Write -Msg "$($GCC) -o $($PARTITIONOUT) -T $(Join-Path $PARTITIONSRC '/partition.ld') --strip-all $($Objects -join ' ')" -color Blue
$GCCOUT = & $GCC '-o' $PARTITIONOUT '-T' (Join-Path $PARTITIONSRC '/partition.ld') '--strip-all' $Objects
Log-Write "$($GCCOUT -join "`n")"
Set-Content $Image $PARTITIONOUT