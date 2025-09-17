param(
    [Parameter(Mandatory=$false)]
    [bool]$RunBochs
)

$TRUE_COMPILE = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\compile.ps1"
$BOOT1 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot1.asm"
$BOOT2 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot2.asm"
$LinkerScript = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot_linker.ld"

if($RunBochs){
    if($RunBochs -eq $true){
        & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2 -LinkerScript $LinkerScript -Bochs_Src "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\.bochsrc" -Run_Bochs 1
    }else{
        & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2 -LinkerScript $LinkerScript -Run 1
    }
}else{
    & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2 -LinkerScript $LinkerScript -Run 1
}