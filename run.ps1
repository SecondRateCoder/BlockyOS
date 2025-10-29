# ./run.ps1 -extrafiles \a0x2800, C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\public\textual.txt -BroadImage -SectorNum 2000 -clear -runbochs

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
    [int]$SectorNum,
    [Parameter(Mandatory=$false)]
    [int]$Reserved,
    [Parameter(Mandatory=$false)]
    [int]$Hidden
)

$TRUE_COMPILE = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\compile.ps1"
$BOOT1 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot1.asm"
$RESERVED_STR = "\x$($Reserved.ToString())"
$HIDDEN_STR = "\x$($Hidden.ToString())"
$FATMAP = "\x18"
$BOOT2 = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot2.asm"
$LinkerScript = "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\src\Boot\boot_linker.ld"

if($clear -eq $true){
    Remove-Item (Join-Path (Get-Location) "Build") -Force -Recurse
}
if($runbochs -eq $true){
    & $TRUE_COMPILE -AsmFiles $BOOT1, $BOOT2, $RESERVED_STR, $HIDDEN_STR, $FATMAP -LinkerScript $LinkerScript -Run_Bochs -ExtraFiles $extrafiles -BroadImage $BroadImage -SectorNum $SectorNum
}elseif ($run -eq $true){
    & $TRUE_COMPILE -AsmFiles $BOOT1, $RESERVED_STR, $HIDDEN_STR, $FATMAP, $BOOT2 -LinkerScript $LinkerScript -Run -ExtraFiles $extrafiles -BroadImage $BroadImage -SectorNum $SectorNum
}else{
    & $TRUE_COMPILE -AsmFiles $BOOT1, $RESERVED_STR, $HIDDEN_STR, $FATMAP, $BOOT2 -LinkerScript $LinkerScript -ExtraFiles $extrafiles -BroadImage $BroadImage -SectorNum $SectorNum
}