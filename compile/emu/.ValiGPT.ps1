param(
    [Parameter(Mandatory)]
    [string]$ImagePath,

    [int]$SectorSize = 512
)

# ================== COMMON HELPERS ==================

function Read-Bytes {
    param(
        [System.IO.FileStream]$Stream,
        [int64]$Offset,
        [int]$Count
    )
    $buf = New-Object byte[] $Count
    $Stream.Seek($Offset, 'Begin') | Out-Null
    $read = $Stream.Read($buf, 0, $Count)
    if($read -ne $Count){
        throw "Short read at offset $Offset(wanted $Count, got $read)"
    }
    $buf
}

function Get-UInt32LE {
    param([byte[]]$Bytes, [int]$Offset)
    [BitConverter]::ToUInt32($Bytes, $Offset)
}

function Get-UInt64LE {
    param([byte[]]$Bytes, [int]$Offset)
    [BitConverter]::ToUInt64($Bytes, $Offset)
}

function Compute-CRC32 {
    param([byte[]]$Data)

    $table = New-Object uint32[] 256
    for($i = 0; $i -lt 256; $i++){
        $crc = [uint32]$i
        for($j = 0; $j -lt 8; $j++){
            if($crc -band 1){
                $crc =(0xEDB88320 -bxor($crc -shr 1))
            } else {
                $crc =($crc -shr 1)
            }
        }
        $table[$i] = $crc
    }

    $crc32 = -bnot [uint32]0
    foreach($b in $Data){
        $idx =($crc32 -bxor $b) -band 0xFF
        $crc32 =($crc32 -shr 8) -bxor $table[$idx]
    }
    -bnot $crc32
}

function Format-GuidLE {
    param([byte[]]$Bytes, [int]$Offset)

    $d1 = [BitConverter]::ToUInt32($Bytes, $Offset)
    $d2 = [BitConverter]::ToUInt16($Bytes, $Offset + 4)
    $d3 = [BitConverter]::ToUInt16($Bytes, $Offset + 6)
    $d4 = $Bytes[($Offset + 8) ..($Offset + 15)]

    $guidBytes = [byte[]](
        [BitConverter]::GetBytes($d1) +
        [BitConverter]::GetBytes($d2) +
        [BitConverter]::GetBytes($d3) +
        $d4
    )

    [Guid]::new($guidBytes)
}

# ================== MBR VALIDATION ==================

function Validate-MBR {
    param([byte[]]$MBR)

    $sigOK =($MBR[510] -eq 0x55 -and $MBR[511] -eq 0xAA)

    $partTable = $MBR[446..509]
    $entries = @()
    for($i = 0; $i -lt 4; $i++){
        $e = $partTable[($i*16)..(($i*16)+15)]
        $type = [byte]$e[4]
        $lbaStart = [BitConverter]::ToUInt32($e, 8)
        $sectors  = [BitConverter]::ToUInt32($e, 12)
        $entries += [pscustomobject]@{
            Index    = $i
            Type     = $type
            LbaStart = $lbaStart
            Sectors  = $sectors
        }
    }

    $protective = $entries | Where-Object { $_.Type -eq 0xEE }

    [pscustomobject]@{
        SignatureOK     = $sigOK
        ProtectiveFound =($null -ne $protective)
        Partitions      = $entries
    }
}

# ================== GPT VALIDATION ==================

function Parse-GptHeader {
    param([byte[]]$Sector)

    $sig = [System.Text.Encoding]::ASCII.GetString($Sector, 0, 8)
    $rev = Get-UInt32LE $Sector 8
    $hdrSize = Get-UInt32LE $Sector 12
    $hdrCRC  = Get-UInt32LE $Sector 16
    $currentLBA = Get-UInt64LE $Sector 24
    $backupLBA  = Get-UInt64LE $Sector 32
    $firstUsable = Get-UInt64LE $Sector 40
    $lastUsable  = Get-UInt64LE $Sector 48
    $diskGuid    = Format-GuidLE $Sector 56
    $partEntriesLBA = Get-UInt64LE $Sector 72
    $numEntries     = Get-UInt32LE $Sector 80
    $entrySize      = Get-UInt32LE $Sector 84
    $entriesCRC     = Get-UInt32LE $Sector 88

    [pscustomobject]@{
        Signature      = $sig
        Revision       = $rev
        HeaderSize     = $hdrSize
        HeaderCRC32    = $hdrCRC
        CurrentLBA     = $currentLBA
        BackupLBA      = $backupLBA
        FirstUsableLBA = $firstUsable
        LastUsableLBA  = $lastUsable
        DiskGuid       = $diskGuid
        PartEntriesLBA = $partEntriesLBA
        NumEntries     = $numEntries
        EntrySize      = $entrySize
        EntriesCRC32   = $entriesCRC
        Raw            = $Sector
    }
}

function Validate-GptHeader {
    param([pscustomobject]$Header)

    $sigOK  =($Header.Signature -eq 'EFI PART')
    $sizeOK =($Header.HeaderSize -ge 92 -and $Header.HeaderSize -le 512)

    $raw = $Header.Raw.Clone()
    $raw[16] = 0
    $raw[17] = 0
    $raw[18] = 0
    $raw[19] = 0

    $calc = Compute-CRC32 $raw[0..($Header.HeaderSize-1)]

    [pscustomobject]@{
        SignatureOK = $sigOK
        SizeOK      = $sizeOK
        CRC32OK     =($calc -eq $Header.HeaderCRC32)
        CalcCRC32   = $calc
    }
}

function Read-PartitionEntries {
    param(
        [System.IO.FileStream]$Stream,
        [pscustomobject]$Header,
        [int]$SectorSize
    )

    $totalBytes = $Header.NumEntries * $Header.EntrySize
    $offset = $Header.PartEntriesLBA * $SectorSize
    Read-Bytes -Stream $Stream -Offset $offset -Count $totalBytes
}

function Validate-PartitionEntries {
    param(
        [byte[]]$Entries,
        [pscustomobject]$Header
    )

    $calc = Compute-CRC32 $Entries
    $crcOK =($calc -eq $Header.EntriesCRC32)

    $list = @()
    for($i = 0; $i -lt $Header.NumEntries; $i++){
        $off = $i * $Header.EntrySize
        $typeGuid = Format-GuidLE $Entries $off
        if($typeGuid -eq [Guid]::Empty){ continue }

        $uniqGuid = Format-GuidLE $Entries($off + 16)
        $firstLBA = Get-UInt64LE $Entries($off + 32)
        $lastLBA  = Get-UInt64LE $Entries($off + 40)
        $attrs    = Get-UInt64LE $Entries($off + 48)
        $nameBytes = $Entries[($off + 56)..($off + $Header.EntrySize - 1)]
        $name = -join($nameBytes | Where-Object { $_ -ne 0 } | ForEach-Object {[char]$_})

        $list += [pscustomobject]@{
            Index     = $i
            TypeGuid  = $typeGuid
            UniqueGuid= $uniqGuid
            FirstLBA  = $firstLBA
            LastLBA   = $lastLBA
            Attributes= $attrs
            Name      = $name
        }
    }

    [pscustomobject]@{
        CRC32OK   = $crcOK
        CalcCRC32 = $calc
        Entries   = $list
    }
}

# ================== FS PROBE ==================

function Probe-Filesystem {
    param(
        [System.IO.FileStream]$Stream,
        [int64]$StartLBA,
        [int]$SectorSize
    )

    $boot = Read-Bytes -Stream $Stream -Offset($StartLBA * $SectorSize) -Count $SectorSize

    $oem = [System.Text.Encoding]::ASCII.GetString($boot, 3, 8).Trim()
    $fatSig1 = [System.Text.Encoding]::ASCII.GetString($boot, 54, 8).Trim()
    $fatSig2 = [System.Text.Encoding]::ASCII.GetString($boot, 82, 8).Trim()
    $ntfs = [System.Text.Encoding]::ASCII.GetString($boot, 3, 4)

    if($ntfs -eq 'NTFS'){
        'NTFS'
    } elseif($fatSig1 -like 'FAT*' -or $fatSig2 -like 'FAT*'){
        'FAT'
    } elseif($oem -like 'MSDOS*' -or $oem -like 'MSWIN*'){
        'FAT-like'
    } else {
        'Unknown'
    }
}

# ================== FAT16/FAT32 VALIDATION ==================

function Parse-FATBootSector {
    param([byte[]]$Boot)

    $bytesPerSector    = [BitConverter]::ToUInt16($Boot, 11)
    $sectorsPerCluster = $Boot[13]
    $reservedSectors   = [BitConverter]::ToUInt16($Boot, 14)
    $fatCount          = $Boot[16]
    $rootEntryCount    = [BitConverter]::ToUInt16($Boot, 17)
    $totalSectors16    = [BitConverter]::ToUInt16($Boot, 19)
    $fatSize16         = [BitConverter]::ToUInt16($Boot, 22)
    $totalSectors32    = [BitConverter]::ToUInt32($Boot, 32)
    $fatSize32         = [BitConverter]::ToUInt32($Boot, 36)
    $rootCluster       = [BitConverter]::ToUInt32($Boot, 44)

    $fatSize = if($fatSize16 -ne 0){ $fatSize16 } else { $fatSize32 }
    $totalSectors = if($totalSectors16 -ne 0){ $totalSectors16 } else { $totalSectors32 }

    $rootDirSectors = [math]::Ceiling(($rootEntryCount * 32) / $bytesPerSector)
    $fatType = if($rootEntryCount -ne 0){ "FAT16" } else { "FAT32" }

    [pscustomobject]@{
        Type              = $fatType
        BytesPerSector    = $bytesPerSector
        SectorsPerCluster = $sectorsPerCluster
        ReservedSectors   = $reservedSectors
        FATCount          = $fatCount
        FATSize           = $fatSize
        TotalSectors      = $totalSectors
        RootEntryCount    = $rootEntryCount
        RootDirSectors    = $rootDirSectors
        RootCluster       = $rootCluster
    }
}

function Read-FAT {
    param(
        [System.IO.FileStream]$Stream,
        [int64]$FatLBA,
        [int]$FatSize,
        [int]$SectorSize
    )

    $bytes = $FatSize * $SectorSize
    Read-Bytes -Stream $Stream -Offset($FatLBA * $SectorSize) -Count $bytes
}

function Get-FAT16NextCluster {
    param(
        [byte[]]$FAT,
        [int]$Cluster
    )
    $offset = $Cluster * 2
    $val = [BitConverter]::ToUInt16($FAT, $offset)
    if($val -ge 0xFFF8){ return -1 }
    $val
}

function Get-FAT32NextCluster {
    param(
        [byte[]]$FAT,
        [int]$Cluster
    )
    $offset = $Cluster * 4
    $val = [BitConverter]::ToUInt32($FAT, $offset) -band 0x0FFFFFFF
    if($val -ge 0x0FFFFFF8){ return -1 }
    $val
}

function Cluster-To-LBA {
    param(
        [int]$Cluster,
        [int64]$DataRegionLBA,
        [int]$SectorsPerCluster
    )
    $DataRegionLBA +(($Cluster - 2) * $SectorsPerCluster)
}

function Validate-FAT-Structure {
    param(
        [pscustomobject]$Boot,
        [int64]$PartitionSectors
    )

    $ok = $true

    if($Boot.BytesPerSector -ne 512 -and $Boot.BytesPerSector -ne 4096){
        Write-Host "    [WARN] Unusual BytesPerSector=$($Boot.BytesPerSector)"
    }

    if($Boot.SectorsPerCluster -lt 1 -or $Boot.SectorsPerCluster -gt 128){
        Write-Host "    [ERR] Invalid SectorsPerCluster=$($Boot.SectorsPerCluster)"
        $ok = $false
    }

    $fatRegion = $Boot.FATCount * $Boot.FATSize
    $used = $Boot.ReservedSectors + $fatRegion + $Boot.RootDirSectors
    if($used -gt $PartitionSectors){
        Write-Host "    [ERR] FAT/Reserved/Root exceed partition size"
        $ok = $false
    }

    if($Boot.TotalSectors -ne 0 -and $Boot.TotalSectors -ne $PartitionSectors){
        Write-Host "    [WARN] BPB TotalSectors=$($Boot.TotalSectors) != partition=$PartitionSectors"
    }

    $ok
}

function Read-FAT32Directory{
    param(
        [System.IO.FileStream]$Stream,
        [byte[]]$FAT,
        [int64]$DataRegionLBA,
        [int]$SectorSize,
        [int]$SectorsPerCluster,
        [int]$Cluster,
        [string]$Path = "",
        [System.Collections.Generic.HashSet[int]]$Visited = $(New-Object 'System.Collections.Generic.HashSet[int]')
    )

    if($Visited.Contains($Cluster)){
        Write-Host "    [ERR] Cluster loop detected at cluster $Cluster(path '$Path')"
        return
    }
    $Visited.Add($Cluster) | Out-Null

    $clusterSizeBytes = $SectorsPerCluster * $SectorSize

    while($Cluster -ne -1){
        $clusterLBA = Cluster-To-LBA -Cluster $Cluster -DataRegionLBA $DataRegionLBA -SectorsPerCluster $SectorsPerCluster
        $buf = Read-Bytes -Stream $Stream -Offset($clusterLBA * $SectorSize) -Count $clusterSizeBytes

        for($i = 0; $i -lt $clusterSizeBytes; $i += 32){
            $entry = $buf[$i..($i+31)]

            if($entry[0] -eq 0x00){return}
            if($entry[0] -eq 0xE5){continue}

            $attr = $entry[11]
            if($attr -band 0x0F -eq 0x0F){continue}

            # $name =([System.Text.Encoding]::ASCII.GetString($entry, 0, 11)).Trim()
            $raw = [System.Text.Encoding]::ASCII.GetString($entry, 0, 11)
            $base = $raw.Substring(0,8).Trim()
            $ext  = $raw.Substring(8,3).Trim()
            $name = if($ext){"$base.$ext"}else{$base}
            $firstCluster =([BitConverter]::ToUInt16($entry, 0x14) -shl 16) -bor [BitConverter]::ToUInt16($entry, 26)
            $size = [BitConverter]::ToUInt32($entry, 28)

            $fullPath = if($Path -eq ""){$name}else{"$Path/$name"}

            Write-Host("    DEBUG ENTRY: {0} firstCluster={1}" -f $fullPath, $firstCluster)

            if($attr -band 0x10){
                Write-Host "DIR  $fullPath"
                if($name -ne "." -and $name -ne ".." -and $firstCluster -ge 2){
                    Read-FAT32Directory -Stream $Stream -FAT $FAT `
                        -DataRegionLBA $DataRegionLBA -SectorSize $SectorSize `
                        -SectorsPerCluster $SectorsPerCluster `
                        -Cluster $firstCluster -Path $fullPath -Visited $Visited
                }
            }else{Write-Host("FILE $fullPath ({0} bytes)" -f $size)}
        }

        $Cluster = Get-FAT32NextCluster -FAT $FAT -Cluster $Cluster
    }
}

# ================== MAIN ==================

$fs = [System.IO.File]::Open($ImagePath, 'Open', 'ReadWrite', 'Read')
try{
    $length = $fs.Length
    $totalLBAs = [int64]($length / $SectorSize)

    Write-Host "Image: $ImagePath"
    Write-Host "Size : $length bytes($totalLBAs sectors of $SectorSize bytes)"
    Write-Host ""

    # --- MBR ---
    $mbr = Read-Bytes -Stream $fs -Offset 0 -Count $SectorSize
    $mbrInfo = Validate-MBR $mbr
    Write-Host "MBR:"
    Write-Host "  Signature OK     : $($mbrInfo.SignatureOK)"
    Write-Host "  Protective Found : $($mbrInfo.ProtectiveFound)"
    foreach($p in $mbrInfo.Partitions){
        Write-Host("  Part{0}: Type={1} LBA={2} Sectors={3}" -f $p.Index,('0x{0:X2}' -f $p.Type), $p.LbaStart, $p.Sectors)
    }
    Write-Host ""

    if(-not($mbrInfo.SignatureOK -and $mbrInfo.ProtectiveFound)){
        Write-Host "[ERR] MBR/Protective MBR invalid; aborting GPT validation."
        return
    }

    # --- Primary GPT ---
    $gptSector = Read-Bytes -Stream $fs -Offset $SectorSize -Count $SectorSize
    $gpt = Parse-GptHeader $gptSector
    $gptVal = Validate-GptHeader $gpt

    Write-Host "Primary GPT Header:"
    Write-Host "  Signature        : $($gpt.Signature)(OK=$($gptVal.SignatureOK))"
    Write-Host "  HeaderSize       : $($gpt.HeaderSize)(OK=$($gptVal.SizeOK))"
    Write-Host("  HeaderCRC32 OK   : $($gptVal.CRC32OK)(Calc=0x{0:X8}, Stored=0x{1:X8})" -f $gptVal.CalcCRC32, $gpt.HeaderCRC32)
    Write-Host "  CurrentLBA       : $($gpt.CurrentLBA)"
    Write-Host "  BackupLBA        : $($gpt.BackupLBA)"
    Write-Host "  FirstUsableLBA   : $($gpt.FirstUsableLBA)"
    Write-Host "  LastUsableLBA    : $($gpt.LastUsableLBA)"
    Write-Host "  Disk GUID        : $($gpt.DiskGuid)"
    Write-Host "  PartEntriesLBA   : $($gpt.PartEntriesLBA)"
    Write-Host "  NumEntries       : $($gpt.NumEntries)"
    Write-Host "  EntrySize        : $($gpt.EntrySize)"
    Write-Host("  EntriesCRC32     : 0x{0:X8}" -f $gpt.EntriesCRC32)
    Write-Host ""

    $entriesBuf = Read-PartitionEntries -Stream $fs -Header $gpt -SectorSize $SectorSize
    $entriesInfo = Validate-PartitionEntries -Entries $entriesBuf -Header $gpt

    Write-Host "Partition Entries:"
    Write-Host("  CRC32 OK         : $($entriesInfo.CRC32OK)(Calc=0x{0:X8}, Stored=0x{1:X8})" -f $entriesInfo.CalcCRC32, $gpt.EntriesCRC32)
    foreach($e in $entriesInfo.Entries){
        Write-Host("  [{0}] {1}  LBA={2}-{3}  Name='{4}'" -f $e.Index, $e.TypeGuid, $e.FirstLBA, $e.LastLBA, $e.Name)
    }
    Write-Host ""

    # --- Backup GPT ---
    $backupHdrOffset = $gpt.BackupLBA * $SectorSize
    $backupSector = Read-Bytes -Stream $fs -Offset $backupHdrOffset -Count $SectorSize
    $gptBackup = Parse-GptHeader $backupSector
    $gptBackupVal = Validate-GptHeader $gptBackup

    Write-Host "Backup GPT Header:"
    Write-Host "  Signature        : $($gptBackup.Signature)(OK=$($gptBackupVal.SignatureOK))"
    Write-Host "  HeaderCRC32 OK   : $($gptBackupVal.CRC32OK)"
    Write-Host "  CurrentLBA       : $($gptBackup.CurrentLBA)"
    Write-Host "  BackupLBA        : $($gptBackup.BackupLBA)"
    Write-Host ""

    # --- Filesystem + FAT integrity ---
    Write-Host "Filesystem Probes:"
    foreach($e in $entriesInfo.Entries){
        $fsType = Probe-Filesystem -Stream $fs -StartLBA $e.FirstLBA -SectorSize $SectorSize
        Write-Host("  [{0}] LBA={1}-{2} Name='{3}' FS={4}" -f $e.Index, $e.FirstLBA, $e.LastLBA, $e.Name, $fsType)

        if($fsType -eq "FAT" -or $fsType -eq "FAT-like"){
            $partSectors = [int64]($e.LastLBA - $e.FirstLBA + 1)

            $boot = Read-Bytes -Stream $fs -Offset($e.FirstLBA * $SectorSize) -Count $SectorSize
            $fatBoot = Parse-FATBootSector $boot

            Write-Host("    FAT Type={0} BytesPerSector={1} SectorsPerCluster={2} Reserved={3} FATs={4} FATSize={5} TotalSectors={6}" -f `
                $fatBoot.Type, $fatBoot.BytesPerSector, $fatBoot.SectorsPerCluster, $fatBoot.ReservedSectors, $fatBoot.FATCount, $fatBoot.FATSize, $fatBoot.TotalSectors)

            $fatOK = Validate-FAT-Structure -Boot $fatBoot -PartitionSectors $partSectors
            Write-Host "    FAT Structure OK: $fatOK"

            if($fatBoot.Type -eq "FAT32"){
                $fatLBA        = $e.FirstLBA + $fatBoot.ReservedSectors
                $FAT           = Read-FAT -Stream $fs -FatLBA $fatLBA -FatSize $fatBoot.FATSize -SectorSize $SectorSize
                $dataRegionLBA = $e.FirstLBA + $fatBoot.ReservedSectors +($fatBoot.FATCount * $fatBoot.FATSize)

                Write-Host "    FAT32 Root Directory:"
                Write-Host("    DEBUG: RootCluster={0} DataRegionLBA={1} SectorsPerCluster={2}" -f $fatBoot.RootCluster, $dataRegionLBA, $fatBoot.SectorsPerCluster)

                Read-FAT32Directory -Stream $fs -FAT $FAT `
                    -DataRegionLBA $dataRegionLBA -SectorSize $SectorSize `
                    -SectorsPerCluster $fatBoot.SectorsPerCluster `
                    -Cluster $fatBoot.RootCluster -Path ""
            }
        }
    }

}finally{$fs.Dispose()}
