param(
    [string]$run,
    [switch]$nocompile,
    [string]$imagetype = 'default'
)

$PREFIX = (Get-Date -Format "yyyy-MM-dd-ss").ToString()

if($run -eq "bochs"){
    & (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -prefix $PREFIX -runbochs  -enablevars -imagetype $imagetype
}elseif($run -eq 'qemu'){
    & (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -run -emudebug -prefix $PREFIX       -enablevars -imagetype $imagetype
}else{
    if($nocompile){
        & (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -prefix $PREFIX -nocompile -imagetype $imagetype
    }else{& (Join-Path (Get-Location) "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -emudebug -prefix $PREFIX -enablevars -imagetype $imagetype}
}