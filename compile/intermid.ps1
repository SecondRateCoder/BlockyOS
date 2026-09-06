param(
    [string]$run,
    [switch]$nocompile,
    [string]$imagetype = 'default'
)

# $PREFIX = (Get-Date -Format "yyyy-MM-dd-ss").ToString()
$PREFIX = "nodate"

if($run -eq "bochs"){
    & (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -emudebug -prefix $PREFIX -runbochs  -enablevars -imagetype $imagetype -InstallOS
}elseif($run -eq 'qemu'){
    & (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -run -emudebug -prefix $PREFIX       -enablevars -imagetype $imagetype -InstallOS
}else{
    if($nocompile){
        & (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -prefix $PREFIX -nocompile -imagetype $imagetype -InstallOS
    }else{& (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -prefix $PREFIX -enablevars -imagetype $imagetype -InstallOS}
}