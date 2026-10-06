param(
	[Parameter(Mandatory=$true)]
	[string]$PREFIX,
	[Parameter(Mandatory=$true)]
	[string]$NASM,
	[Parameter(Mandatory=$true)]
	[string]$GCC,
	[Parameter(Mandatory=$true)]
	[string]$EMUOUT,
	[string]$ARCHITECTURE = 'x86_64',
	[string]$layoutjson,
	[bool]$ENABLEDEBUGGABLE,
	[switch]$ENABLENTEMULATOR
)
(& (Join-Path (Get-Location) '/src/Boot/UEFI/GUID.ps1') -jsonpath $layoutjson)

$GCC = Join-Path (Get-Location) 'src/Boot/UEFI/gcc.ps1'

$EFIPARENT = Join-Path (Get-Location) "compile\toolchain\gnu-efi-build\$($ARCHITECTURE)\"
$BUILDDIR = Join-Path (Get-Location) "Build\Build-$($PREFIX)\"
$OBJDIR = Join-Path $BUILDDIR 'objs/'
$LOGFILE = Join-Path $BUILDDIR 'uefi.log'
$UEFIINTERMEDIATEFINAL = (Join-Path -Path $Objdir "blob_2.so")
$UEFIBINARYBLOB = (Join-Path -Path $Objdir "blob.efi")
$UEFIDebuggingBLOB = (Join-Path -Path $Objdir "debug.efi")
$EMUUEFIBINARYDIR = (Join-Path -Path $EMUOUT "/EFI/BOOT/")
$EMUUEFIBINARYBLOB = (Join-Path -Path $EMUUEFIBINARYDIR "/BOOTX64.EFI")
$OBJCOPY = 'objcopy'

$TEMPCACHE = Join-Path (Get-Location) 'compile/cache/'
$TEMPJSON = Join-Path $TEMPCACHE '/cache.json'
if(-not (Test-Path $TEMPCACHE)){New-Item $TEMPCACHE -ItemType Directory -Force}
if(-not (Test-Path $TEMPJSON)){New-Item $TEMPJSON -ItemType File -Force}

$CARGS = @(
	'-I', $TEMPCACHE, '-I', "$(Get-Location)/src/", '-I', "$(Get-Location)/", '-I', "$($EFIPARENT)\include\efi\", 
	'-I', (Join-Path (Get-Location) "compile\toolchain\prebuild\include\"), '-I',"$($EFIPARENT)include\efi\legacy\",
	'-I', "$($EFIPARENT)include\efi\$(if($ARCHITECTURE -eq 'x86_64'){'x86_64'}else{'ia32'})\", 
	'-fshort-wchar', '-fPIE', '-fPIC', '-maccumulate-outgoing-args', '-fno-omit-frame-pointer', 
	"-m$(if($ARCHITECTURE -eq 'x86_64'){'64'}else{'32'})",
	'-D', "$(if($ARCHITECTURE -eq 'x86_64'){'__x86_64__', '-mno-red-zone'}else{'__ia32__', '-D', 'EFI32'})", 
	'-D', '_DEBUG', '-D', '__CUSTMEM_FUNC__', '-D', 'NATIVE_LITTLE_ENDIAN'
)
if($ENABLEDEBUGGABLE){$CARGS += '-D', 'EFI_DEBUG', '-g'}
if($ENABLENTEMULATOR){$CARGS += '-D', 'EFI_NT_EMULATOR'}
# if($ARCHITECTURE -eq 'x86_64'){$CARGS += '-D', '__x86_64__'}
# elseif($ARCHITECTURE -eq 'x86'){$CARGS += '-D', 'EFI32', '-D', '__ia32__'}

$LARGSL = @(
	'-shared', '-Bsymbolic', '-znocombreloc', '-z', 'noexecstack', 
	'-T', "$($EFIPARENT)lib/elf_$($ARCHITECTURE)_efi.lds", 
	"$($EFIPARENT)lib/crt0-efi-$($ARCHITECTURE).o", 
	'-L', "$($EFIPARENT)/lib/", '-l', 'efi', '-l', 'gnuefi'
)
$OBJCOPYARGS = @(
	'-j', '.text', '-j', '.data', '-j', '.bss', '-j', '.rdata', '-j', '.rodata', 
	'-j', '.sbss', '-j', '.sdata', '-j', '.srdata', '-j', '.dynamic', '-j', '.dynsym', 
    '-j', '.rel', '-j', '.rel.*', '-j', '.rela', '-j', '.rela.*', , '-j', '.idata', 
    '-j', '.reloc', '-O', "efi-app-$(if($ARCHITECTURE -eq 'x86_64'){'x86-64'}else{'i386'})", 
	'--subsystem=10', $UEFIINTERMEDIATEFINAL, $UEFIBINARYBLOB
)
$OBJCOPYARGS2 = @('--only-keep-debug', $UEFIINTERMEDIATEFINAL, $UEFIDebuggingBLOB)

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
	if(-not (Test-Path $LOGFILE)){
		New-Item $BUILDDIR -ItemType Directory -ErrorAction SilentlyContinue
		New-Item $LOGFILE -ItemType File
	}
	$success = $false
	do{
		$success = $true
		try{
			if(-not (Test-Path $LOGFILE)){New-Item $LOGFILE -ItemType File}
			Add-Content -Path $LOGFILE -Value $clean
		}catch{$success = $true}
	}while($success -eq $false)
}

function Ensure-PosixUefiRuntime {
	$POSIXUEFI_LIB = "$($EFIPARENT)/lib"
	if(-not (Test-Path $POSIXUEFI_LIB)){
		Log-Write "GNU-EFI library does not exist: $POSIXUEFI_LIB"
		throw ''
	}
}

(Ensure-PosixUefiRuntime)

# Compile all UEFI .c files
$SourceFiles = @()
Get-ChildItem -Path @((Join-Path (Get-Location) "src/Boot/UEFI/")) -Include @("*.c", "*.s", "*.asm") -Recurse -File | ForEach-Object{$SourceFiles += $_.FullName}

(& 'tools\build-suite\gcc.ps1' -CacheEnabled -LogEnabled -f $SourceFiles -o $UEFIINTERMEDIATEFINAL -Prefix 'UEFI' -Toolchain 'elf' -c $CARGS -l $LARGSl -CACHEDIR $TEMPCACHE -LogFile $LOGFILE)
# Log-Write "gcc $($LARGSU -join ' ') $($OFILES -join ' ') -o $($UEFIINTERMEDIATESRC)" -color Blue
# $LDOUT = & $GCC -_ARGS @($LARGSU, $OFILES, '-o', $UEFIINTERMEDIATESRC)
# Log-Write "gcc $($UEFIINTERMEDIATESRC) $($LARGSL -join ' ') -o $($UEFIINTERMEDIATEFINAL)" -color Blue
# $LDOUT = & $GCC -_ARGS @($UEFIINTERMEDIATESRC, $LARGSL, '-o', $UEFIINTERMEDIATEFINAL)
# Log-Write "$($LDOUT -join "`n")"
if(Test-Path $UEFIINTERMEDIATEFINAL){
	try{Log-Write "$(& $OBJCOPY '-V')"}catch{
		Log-Write -Msg "Objcopy not found: $OBJCOPY" -color Red
		exit 1
	}
	Log-Write "$($OBJCOPY) $($OBJCOPYARGS -join ' ')" -color Blue
    $OBJCOPYOUT = (& $OBJCOPY $OBJCOPYARGS)
	Log-Write "$($OBJCOPY) $($OBJCOPYARGS2 -join ' ')" -color Blue
    $OBJCOPYOUT += @("`n`n", (& $OBJCOPY $OBJCOPYARGS2))
    Log-Write "$($OBJCOPYOUT -join "`n")"
}
if(Test-Path $UEFIBINARYBLOB){
	New-Item $EMUUEFIBINARYDIR -ItemType Directory -Force
	Copy-Item -Path $UEFIBINARYBLOB -Destination $EMUUEFIBINARYBLOB
}

(Copy-Item (Join-Path (Get-Location) 'src/Boot/UEFI/startup.sh') (Join-Path $EMUOUT 'startup.nsh'))
(& objdump '-x' $UEFIBINARYBLOB) >> (Join-Path $BUILDDIR 'headerxdump.log')