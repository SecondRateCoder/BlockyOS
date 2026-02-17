param(
    [string]$run
)

if($run -eq "bochs"){&(Join-Path -Path (Get-Location) -ChildPath "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -runbochs
}else{&(Join-Path -Path (Get-Location) -ChildPath "\compile\run.ps1") -BroadImage -SectorNum (1024 * 1024) -clear -run}