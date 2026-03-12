param([Parameter(Mandatory=$false)]$OUTEXE = "util.exe")


$GCC = "gcc"
$BUILDOUT = Join-Path (Get-Location) "//tools//build//"
$OBJOUT = Join-Path $BUILDOUT "//obj//util//"
$LOGFILE = Join-Path (Get-Location) "//tools//build//logs//util.log"
$FILES = (Get-ChildItem -Path (Join-Path (Get-Location) "//tools//utility//") -Filter "*.c" -Recurse -File)

if(Test-Path (Join-Path $BUILDOUT $OUTEXE)){Remove-Item (Join-Path $BUILDOUT $OUTEXE) -Force}
if(-not(Test-Path $BUILDOUT)){New-Item $BUILDOUT -Force -ItemType Directory}
if(-not(Test-Path $OBJOUT)){New-Item $OBJOUT -Force -ItemType Directory}
New-Item $LOGFILE -Force -ItemType File


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
    if(-not (Test-Path $Log)){New-Item $Log -ItemType File}
    $success = $false
    do{
        $success = $true
        try{
            if(-not (Test-Path $Log)){New-Item $Log -ItemType File}
            Add-Content -Path $Log -Value $clean
        }catch{$success = $true}
    }while($success -eq $false)
}

$OBJFILE = @()
$FILES|ForEach-Object{
    $FOUT = Join-Path $OBJOUT "$($_.BaseName).o"
    if(Test-Path $FOUT){Remove-Item $FOUT -Force}
    $OBJFILE += $FOUT
    Log-Write "$($GCC) -c $($_.FullName) -o $($FOUT) -g -I $(Join-Path (Get-Location) "//tools//utility//")" -color Blue
    $GCCOUT = & $GCC '-fdiagnostics-color=always' '-c' $_.FullName '-o' $FOUT '-g' '-I' (Join-Path (Get-Location) "//tools//utility//") 2>&1
    Log-Write ($GCCOUT -join "`n")
}

Log-Write "$($GCC) -o $(Join-Path $BUILDOUT $OUTEXE) $($OBJFILE -join "`n")" -color Blue
$LINKOUT = & $GCC '-o' (Join-Path $BUILDOUT $OUTEXE) $OBJFILE 2>&1
Log-Write ($LINKOUT -join "`n")