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
$BOOT1 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot1.asm"
$BOOT2 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot2.asm"
$LinkerScript = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot_linker.ld"

if($clear -eq $true){
    Remove-Item (Join-Path (Get-Location) "Build") -Force -Recurse
}
if($runbochs -eq $true){
    & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2 -LinkerScript $LinkerScript -Run_Bochs 1 -ExtraFiles $extrafiles -BroadImage $BroadImage -SectorNum $SectorNum
}else{
    & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2 -LinkerScript $LinkerScript -Run $run -ExtraFiles $extrafiles  -BroadImage $BroadImage -SectorNum $SectorNum
}