#* Compile both Boot assembly files and append them to the floppy.img in the build folder

#! Parameters
param(
	[Parameter(Mandatory=$false)]
	[string]$LinkerDir
)

#! Directory setup
$Build = Join-Path (Get-Location) "Build"
$Image = Join-Path $Build ("floppy_" + (Get-Date -Format "yyyy-MM-dd_HH-mm-ss") + ".img")
$Logdir = Join-Path $Build "Logs\"
$Log = Join-Path $logDir ("log_" + (Get-Date -Format "yyyy-MM-dd_HH-mm-ss") + ".txt")
$bootDir = Join-Path (Get-Location) "src\Boot\"

#! Compiler setup
#* Just use gcc and directly enter the assembly stage or use NASM which ever.
$NASM = "nasm"
$LINKER = "ld"

#! Functions
function Log-Write{
    param ([string]$Message)
    Write-Host $Message
    Add-Content -Path $Log -Value $Message
}

function Img-Push {
    param(
        [byte[]]$Data
    )
    
    # Open the file stream in Append mode
    $fs = [System.IO.File]::Open($Image, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write)
    try {
        $fs.Write($Data, 0, $Data.Length)
    } finally {
        $fs.Close()
    }
}

function Test-GccAvailability {
    Log-Write "Checking for GCC availability..."
    try {
        $null = & $gccPath --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            Log-Write "GCC found: $($gccPath)"
            return $true
        } else {
            Log-Write "GCC not found or not callable. Please ensure GCC is installed and in your system's PATH."
            return $false
        }
    } catch {
        Log-Write "Error checking GCC: $($_.Exception.Message)"
        Log-Write "Please ensure GCC is installed and in your system's PATH."
        return $false
    }
}

function Boot_Compile {
	param()
	$asmFiles = Get-ChildItem -Path $bootDir -Filter "*.asm" -Recurse | Select-Object -ExpandProperty FullName
	if($asmFiles){
		foreach($file in $asmFiles){
			$arguments = "-f bin" + $file + [System.IO.Path]::GetFileNameWithoutExtension($file) + ".bin"
			$nasmOutput = & $NASM $arguments 2>&1
			Log-Write $nasmOutput
		}
	}
}

#Links each Boot file then appends them to the Image
function Boot_Link{
	param(
		[string[]]$objPaths
	)
	if($LinkerDir){
		# Use specified Linker
	}else{
		# Use default Linker
	}
}