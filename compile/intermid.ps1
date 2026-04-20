param(
    [string]$run
)

$PREFIX = (Get-Date -Format "yyyy-MM-dd-ss").ToString()
$EXE = Join-Path (Get-Location) ("Build\Build-" + $PREFIX + "\UEFI\objs\blob.efi")
if($run -eq "bochs"){
& (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -EXEdebug $EXE -prefix $PREFIX -runbochs
}elseif($run -eq 'qemu'){
& (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -run -emudebug -EXEdebug $EXE -prefix $PREFIX
}else{
& (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -EXEdebug $EXE -prefix $PREFIX
}