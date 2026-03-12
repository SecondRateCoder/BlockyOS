param(
    [string]$run
)

$prefix = (Get-Date -Format "yyyy-MM-dd-ss").ToString()
$EXE = Join-Path (Get-Location) ("Build\Build-" + $prefix + "\UEFI\objs\blob.efi")
if($run -eq "bochs"){
& (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -EXEdebug $EXE -prefix $prefix -runbochs
}elseif($run -eq 'qemu'){
& (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -run -emudebug -EXEdebug $EXE -prefix $prefix
}else{
& (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -EXEdebug $EXE -prefix $prefix
}