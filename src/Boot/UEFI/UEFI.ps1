param(
	[Parameter(Mandatory=$true)]
	[string]$prefix,
	[Parameter(Mandatory=$true)]
	[string]$NASM,
	[Parameter(Mandatory=$true)]
	[string]$GCCPS1
)

$GCC = 'gcc'

$EFIPARENT = Join-Path (Get-Location) "compile/toolchain/posix-uefi/"
$ARCHITECTURE = "x86_64"
$ARCHEXCLUDE = @('aarch64', 'riscv64')

$Build = Join-Path (Get-Location) ("Build\Build-" + $prefix + "\UEFI\")
if(-not (Test-Path $Build)){New-Item $Build -ItemType Directory}
$Log = Join-Path $Build ("/logUEFI.log")
$Objdir = Join-Path $Build "\objs\"
if(-not (Test-Path $Objdir)){New-Item $Objdir -ItemType Directory}
$Image = Join-Path $Build ("floppy-" + $prefix + ".img")
$MAPFILE = Join-Path $Build "UEFImap.log"
if(-not (Test-Path $MAPFILE)){New-Item $MAPFILE -ItemType File}

$PARTITIONBLOB = Join-Path $Objdir 'partition.bin'
$PARTITIONEXE = Join-Path $Objdir 'partition.efi'
$PARTITIONOUT = Join-Path $Objdir 'partitionblob.o'
$PARTITIONSRC = (Join-Path (Get-Location) "src/Boot/UEFI/Partition")
$PARTITIONMAP = Join-Path $Build '/partitionMap.log'

$Objects = @()
$POBJFILES = @()

$UEFIBINARYBLOB = (Join-Path -Path $Objdir "/blob.efi")
$UEFIMAP = (Join-Path -Path ("Build\Build-" + $prefix + "\objs\") "UEFI/table.json")
[hashtable]$UEFIBINARIESTABLE = @{
	'FUEFI/boot.efi' = $UEFIBINARYBLOB
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
	if(-not (Test-Path $Log)){
		New-Item $Build -ItemType Directory -ErrorAction SilentlyContinue
		New-Item $Log -ItemType File
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

$CARGS = @(
	"-I", "$(Get-Location)/src/",
	"-I", $EFIPARENT, 
	"-nostdlib", "-m64", "-fdiagnostics-color=always"
	"-ffreestanding", "-fshort-wchar", "-mno-red-zone", "-fpic",
	"-fno-stack-protector", "-fno-builtin", "-fno-exceptions"
	# "$(Get-Location)/compile/toolchain/prebuild/i686-elf/include/",
)
$LARGS = @(
	"-o", $UEFIBINARYBLOB, 
	"-nostdlib", "-Wl,--subsystem=10", "-Wl,--image-base,0x00",
	"-Wl,-T,$($EFIPARENT)/uefi/elf_$($ARCHITECTURE)_efi.lds", "-Wl,-Map,$($MAPFILE)",
	"-Wl,--sysroot=$($EFIPARENT)"
)
$PARTITIONCARGS = @(
	"-I", "$(Get-Location)/src/",
	"-I", $EFIPARENT, 
	'-nostdlib', '-fno-stack-protector', '-fno-builtin', '-fno-exceptions',
	'-fdata-sections', '-fno-function-sections', '-Wl,--omagic'
)
$PARTITIONLARGS = @(
	'-o', $PARTITIONEXE, '-nostdlib', '-r', 
	"-Wl,-T,$(Join-Path $PARTITIONSRC '/partition.ld')",
	"-Wl,-Map,$($PARTITIONMAP)"
)

# Compile all UEFI .c files except for those in the Partition folder with those in the UEFI folder
Get-ChildItem -Path @((Join-Path (Get-Location) "src/Boot/UEFI"), "$($EFIPARENT)/uefi") -Include "*.c" -Recurse -File|
Where-Object { $_.FullName -notmatch '\\Partition\\' -and $_.FullName -notmatch '/Partition/'} | ForEach-Object{
	$f = $_
	$cont = $false
	$ARCHEXCLUDE | ForEach-Object{if($f.BaseName -match $_){$cont = $true}}
	if(-not $cont){
		$o = "$(Join-Path $Objdir $_.BaseName).o"
		Log-Write -Msg "$($GCC) -c $($_.FullName) -o $($o) $($CARGS -join ' ')" -color Blue
		$GCCOUT = & $GCC '-c' $_.FullName '-o' $o $CARGS 2>&1
		Log-Write -Msg ($GCCOUT -join "`n")
		if(-not (Test-Path $o)){exit 1}
		$Objects += $o
	}
}
Log-Write -Msg "$($GCC) $($LARGS -join ' ') $($Objects -join ' ')" -color Blue
$GCCOUT = & $GCC $LARGS $Objects
Log-Write -Msg "$($GCCOUT -join "`n")"

# Compile all Files in Partition Folder
Get-ChildItem -Path $PARTITIONSRC -Include "*.c" -Recurse -File | ForEach-Object{
	$o = "$(Join-Path $Objdir $_.BaseName).o"
	Log-Write -Msg "$($GCC) -c $($_.FullName) -o $($o) $($PARTITIONCARGS -join ' ')" -color Blue
	$GCCOUT = & $GCC '-c' $_.FullName '-o' $o $PARTITIONCARGS
	Log-Write -Msg ($GCCOUT -join "`n")
	if(-not (Test-Path $o)){
		Set-Content $Image $PARTITIONOUT
		if(Test-Path $UEFIMAP){Remove-Item (Get-Item $UEFIMAP).Parent.FullName -Force}
		New-Item (Join-Path -Path $Objdir "UEFI/") -ItemType Directory
		New-Item $UEFIMAP -ItemType Directory
		Set-Content $UEFIMAP | ConvertTo-Json $UEFIBINARIESTABLE
	}
	$POBJFILES += $o
}
Log-Write -Msg "$($GCC) $($PARTITIONLARGS -join ' ') $($POBJFILES -join ' ')" -color Blue
$GCCOUT = & $GCC $PARTITIONLARGS $POBJFILES
Log-Write -Msg "$($GCCOUT -join "`n")"
Log-Write -Msg "objcopy -O binary $($PARTITIONEXE) $($PARTITIONBLOB)"
$OBJCOPYOUT = & objcopy.exe '-O' 'binary' $PARTITIONEXE $PARTITIONBLOB
Log-Write -Msg "$($OBJCOPYOUT -join "`n")"

Set-Content -Path $Image -Value (Get-Content $PARTITIONOUT)
if(Test-Path $UEFIMAP){Remove-Item (Get-Item $UEFIMAP).Parent.FullName -Force}
New-Item (Join-Path -Path $Objdir "UEFI/") -ItemType Directory
New-Item $UEFIMAP -ItemType Directory
Set-Content -Path $UEFIMAP -Value (ConvertTo-Json $UEFIBINARIESTABLE)