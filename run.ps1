#./run.ps1 -extrafiles \a0x2800, C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\public\textual.txt -BroadImage -SectorNum 2000 -clear
param(
    # [Parameter(Mandatory=$false)]
    [switch]$runbochs,
    # [Parameter(Mandatory=$false)]
    [switch]$run,
    # [Parameter(Mandatory=$false)]
    [switch]$clear,
    [Parameter(Mandatory=$false)]
    [string[]]$extrafiles,
    [switch]$BroadImage,
    [Parameter(Mandatory=$false)]
    [int]$SectorNum
)

$TRUE_COMPILE = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\compile.ps1"
$FATMAP = "\x18"
$BOOT1 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot1.asm"
$BOOT2 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot2.asm"
$LinkerScript = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot_linker.ld"

if($clear -eq $true){
    Remove-Item (Join-Path (Get-Location) "Build") -Force -Recurse
}
if($runbochs -eq $true){
    & $TRUE_COMPILE -AsmFiles $BOOT1, $FATMAP, $BOOT2 -LinkerScript $LinkerScript -Run_Bochs -ExtraFiles $extrafiles -BroadImage $BroadImage -SectorNum $SectorNum
}elseif ($run -eq $true){
    & $TRUE_COMPILE -AsmFiles $BOOT1, $FATMAP, $BOOT2 -LinkerScript $LinkerScript -Run -ExtraFiles $extrafiles -BroadImage $BroadImage -SectorNum $SectorNum
}else{
    & $TRUE_COMPILE -AsmFiles $BOOT1, $FATMAP, $BOOT2 -LinkerScript $LinkerScript -ExtraFiles $extrafiles -BroadImage $BroadImage -SectorNum $SectorNum
}