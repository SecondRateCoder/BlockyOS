param(
    [Parameter(Mandatory=$true)]
    [string]$jsonpath,
    [string]$bootPartName = "Boot",
    [string]$rootPartName = "Root"
)
function Convert-ToEfiGuid{
    param(
        [Parameter(Mandatory=$true)]
        [Guid]$Guid
    )

    # Break GUID into components
    $bytes = $Guid.ToByteArray()

    # EFI GUID format:
    # { UINT32, UINT16, UINT16, { UINT8[8] } }

    $d1 = [BitConverter]::ToUInt32($bytes, 0)
    $d2 = [BitConverter]::ToUInt16($bytes, 4)
    $d3 = [BitConverter]::ToUInt16($bytes, 6)

    $d4 = $bytes[8..15] | ForEach-Object { "0x{0:X2}" -f $_ }
    $d4str = $d4 -join ","

    # Output EFI_GUID initializer as a string
    return ("(EFI_GUID){{0x{0:X8},0x{1:X4},0x{2:X4},{{{3}}}}}" -f $d1, $d2, $d3, $d4str)
}
$guidfile = Join-Path (Get-Location) '/src/Boot/UEFI/guid.h'

$expanded = Get-Content $jsonpath | ConvertFrom-Json -AsHashtable

$parts = @()
$bootpart = $null
$installpart = $null
foreach($part in $expanded.partitions){
    if($part.name -match $bootPartName){$bootpart = $part}
    elseif($part.name -match $rootPartName){$installpart = $part}
    else{$parts += "(eGUID){`"$($part.name)`", $(Convert-ToEfiGuid -Guid ([guid]$part.type)), $(Convert-ToEfiGuid -Guid ([guid]$part.unique))}"}
}

$guid_h = "
#pragma once

#include `"efi.h`"
#include `"efilib.h`"

typedef struct eGUID{char *name;    EFI_GUID guid, uGuid;}eGUID;

typedef struct localGUID_t{
    EFI_GUID diskID;
    eGUID bootPART, installPART;
    UINTN nGUIDs;
    eGUID guids[];;
}localGUID_t;

static const localGUID_t installDISK = {
    .diskID = $(Convert-ToEfiGuid -Guid ([guid]$expanded.disk.type)),
    .bootPART = {
        `"$($bootpart.name)`", 
        $(Convert-ToEfiGuid -Guid ([guid]$bootpart.type)), 
        $(Convert-ToEfiGuid -Guid ([guid]$bootpart.unique))
    },
    .installPART = {
        `"$($installpart.name)`", 
        $(Convert-ToEfiGuid -Guid ([guid]$installpart.type)), 
        $(Convert-ToEfiGuid -Guid ([guid]$installpart.unique))
    },
    .nGUIDs = $($parts.Length),
    .guids = {
        $($parts -join ", `n`t`t`t")
    }
};

#define bootDesc (installDISK.bootPART)
#define rootDesc (installDISK.installPART)
"

if(Test-Path $guidfile){Remove-Item $guidfile}
(New-Item $guidfile -ItemType File)
(Set-Content -Value $guid_h -Path $guidfile)