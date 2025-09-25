param(
    [Parameter(Mandatory=$false)]
    [bool]$RunBochs,
    [Parameter(Mandatory=$false)]
    [bool]$Run,
    [Parameter(Mandatory=$false)]
    [bool]$Clear
)

$TRUE_COMPILE = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\compile.ps1"
$BOOT1 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot1.asm"
$BOOT2 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot2.asm"
$LinkerScript = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot_linker.ld"

if($Clear){
    Remove-Item (Join-Path (Get-Location) "Build") -Force -Recurse
}else{
    if($RunBochs){
        if($RunBochs -eq $true){
            & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2 -LinkerScript $LinkerScript -Run_Bochs 1
        }else{
            & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2 -LinkerScript $LinkerScript -Run $Run
        }
    }else{
        & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2 -LinkerScript $LinkerScript -Run $Run
    }
}