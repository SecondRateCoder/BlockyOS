param(
    [Parameter(Mandatory=$true, HelpMessage="Array of paths to GCC LD Map files")]
    [string[]]$MapFiles,
    
    [Parameter(Mandatory=$true, HelpMessage="Array of base addresses corresponding to each map file")]
    [string[]]$MapBases,
    
    [Parameter(Mandatory=$true, HelpMessage="Target crash address (e.g., 0x7F91EDBD)")]
    [string]$TargetAddress,

    [Parameter(Mandatory=$false)]
    [string]$LinkerBase = "0x0",

    [Parameter(Mandatory=$false, HelpMessage="Number of closest symbols to return")]
    [int]$Count = 5
)

if ($MapFiles.Length -ne $MapBases.Length) {
    Write-Error "The number of MapFiles must exactly match the number of MapBases provided."
    return
}

$target = [Convert]::ToUInt64($TargetAddress.Replace("0x", ""), 16)
$linkerBaseOffset = [Convert]::ToUInt64($LinkerBase.Replace("0x", ""), 16)

Write-Host "Target Crash Address: 0x$($target.ToString('X'))" -ForegroundColor Cyan

$globalMatches = [System.Collections.Generic.List[PSCustomObject]]::new()

$results = for ($i = 0; $i -lt $MapFiles.Length; $i++) {
    $file = $MapFiles[$i]
    $baseStr = $MapBases[$i]
    
    if(-Not (Test-Path $file)){ 
        Write-Warning "File not found: $file"
        continue 
    }

    $base = [Convert]::ToUInt64($baseStr.Replace("0x", ""), 16)
    
    if ($target -lt $base) {
        [PSCustomObject]@{
            MapFile       = (Get-Item $file).Name
            BaseAddress   = "0x$($base.ToString('X'))"
            ParsedSymbols = 0
            ClosestSymbol = "None"
            SymbolAddr    = "N/A"
            ByteOffset    = "Target before Base"
        }
        continue
    }

    $relativeOffset = $target - $base
    $searchAddress = $relativeOffset + $linkerBaseOffset

    $parsedCount = 0
    $fileMatches = [System.Collections.Generic.List[PSCustomObject]]::new()

    switch -Regex -File $file {
        '^\s*(0x[0-9a-fA-F]+)\s+([a-zA-Z_.$][a-zA-Z0-9_.$]+)' {
            try {
                $symAddr = [Convert]::ToUInt64($matches[1].Replace("0x", ""), 16)
                $parsedCount++
                
                if ($symAddr -le $searchAddress) {
                    $diff = $searchAddress - $symAddr
                    $fileMatches.Add([PSCustomObject]@{
                        MapFile    = (Get-Item $file).Name
                        Symbol     = $matches[2]
                        SymbolAddr = $symAddr
                        Diff       = $diff
                    })
                }
            } catch {}
        }
    }

    $topFileMatches = $fileMatches | Sort-Object Diff | Select-Object -First $Count
    
    # FIX: Use a foreach loop instead of AddRange to bypass PowerShell's strict generic type binding
    if ($topFileMatches) {
        foreach ($match in $topFileMatches) {
            $globalMatches.Add($match)
        }
    }

    # Force $topFileMatches into an array so we can safely index it even if $Count is 1
    $topFileMatchesArray = @($topFileMatches)
    $bestMatch = if ($topFileMatchesArray.Count -gt 0) { $topFileMatchesArray[0] } else { $null }

    [PSCustomObject]@{
        MapFile       = (Get-Item $file).Name
        BaseAddress   = "0x$($base.ToString('X'))"
        ParsedSymbols = $parsedCount
        ClosestSymbol = if ($bestMatch) { $bestMatch.Symbol } else { "None" }
        SymbolAddr    = if ($bestMatch) { "0x$($bestMatch.SymbolAddr.ToString('X'))" } else { "N/A" }
        ByteOffset    = if ($bestMatch) { "+0x$($bestMatch.Diff.ToString('X'))" } else { "N/A" }
    }
}

$results | Format-Table -AutoSize

if ($globalMatches.Count -gt 0) {
    Write-Host "--- TOP $Count CLOSEST OVERALL MATCHES ---" -ForegroundColor Green
    
    $topGlobal = $globalMatches | Sort-Object Diff | Select-Object -First $Count
    
    $rank = 1
    foreach ($match in $topGlobal) {
        Write-Host "$($rank). File: $($match.MapFile) | Symbol: $($match.Symbol) | Offset: +0x$($match.Diff.ToString('X'))" -ForegroundColor Green
        $rank++
    }
    Write-Host ""
}