param(
	[Parameter(Mandatory)]
	[string]$ImagePath,

	[int]$SectorSize = 512
)

# --- Helpers -------------------------------------------------------------

function Read-Bytes {
	param(
		[System.IO.FileStream]$Stream,
		[int64]$Offset,
		[int]$Count
	)
	$buf = New-Object byte[] $Count
	$Stream.Seek($Offset, 'Begin') | Out-Null
	$read = $Stream.Read($buf, 0, $Count)
	if ($read -ne $Count) {
		throw "Short read at offset $Offset (wanted $Count, got $read)"
	}
	return $buf
}

function Get-UInt32LE {
	param([byte[]]$Bytes, [int]$Offset)
	return [BitConverter]::ToUInt32($Bytes, $Offset)
}

function Get-UInt64LE {
	param([byte[]]$Bytes, [int]$Offset)
	return [BitConverter]::ToUInt64($Bytes, $Offset)
}

$crc32table = New-Object uint32[] 256
for($i = 0; $i -lt 256; $i++){
	$crc = [uint32]$i
	for($j = 0; $j -lt 8; $j++){
		if($crc -band 1){$crc = (0xEDB88320 -bxor ($crc -shr 1))
		}else{$crc = ($crc -shr 1)}
	}
	$crc32table[$i] = $crc
}
function Compute-CRC32 {
	param([byte[]]$Data)

	$crc32 = -bnot [uint32]0
	foreach ($b in $Data) {
		$idx = ($crc32 -bxor $b) -band 0xFF
		$crc32 = ($crc32 -shr 8) -bxor $crc32table[$idx]
	}
	$crc32 = -bnot $crc32
	return [uint32]$crc32
}

# function Format-GuidLE {
#     param([byte[]]$Bytes, [int]$Offset)

#     $d1 = [BitConverter]::ToUInt32($Bytes, $Offset)
#     $d2 = [BitConverter]::ToUInt16($Bytes, $Offset + 4)
#     $d3 = [BitConverter]::ToUInt16($Bytes, $Offset + 6)
#     $d4 = $Bytes[($Offset + 8) .. ($Offset + 15)]

#     return New-Object guid ([byte[]](
#         [BitConverter]::GetBytes($d1) +
#         [BitConverter]::GetBytes($d2) +
#         [BitConverter]::GetBytes($d3) +
#         $d4
#     ))
# }
function Format-GuidLE {
	param([byte[]]$Bytes, [int]$Offset)

	$d1 = [BitConverter]::ToUInt32($Bytes, $Offset)
	$d2 = [BitConverter]::ToUInt16($Bytes, $Offset + 4)
	$d3 = [BitConverter]::ToUInt16($Bytes, $Offset + 6)
	$d4 = $Bytes[($Offset + 8) .. ($Offset + 15)]

	$guidBytes = [byte[]](
		[BitConverter]::GetBytes($d1) +
		[BitConverter]::GetBytes($d2) +
		[BitConverter]::GetBytes($d3) +
		$d4
	)

	return [Guid]::new($guidBytes)
}


# --- MBR -----------------------------------------------------------------

function Validate-MBR {
	param(
		[byte[]]$MBR
	)

	$okSig = (($MBR[510..511])[0] -eq 0x55 -and ($MBR[510..511])[1] -eq 0xAA)

	$partTable = $MBR[446..509]
	$entries = @()
	for ($i = 0; $i -lt 4; $i++) {
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

	return [pscustomobject]@{
		SignatureOK      = $okSig
		ProtectiveFound  = ($null -ne $protective)
		Partitions       = $entries
	}
}

# --- GPT Header ---------------------------------------------------------

function Parse-GptHeader {
	param(
		[byte[]]$Sector
	)

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
	param(
		[pscustomobject]$Header
	)

	$sigOK = ($Header.Signature -eq 'EFI PART')
	$sizeOK = ($Header.HeaderSize -ge 92 -and $Header.HeaderSize -le 512)

	$raw = $Header.Raw.Clone()
	# zero CRC field
	$raw[16] = 0
	$raw[17] = 0
	$raw[18] = 0
	$raw[19] = 0

	$calc = Compute-CRC32 $raw[0..($Header.HeaderSize-1)]

	[pscustomobject]@{
		SignatureOK = $sigOK
		SizeOK      = $sizeOK
		CRC32OK     = ($calc -eq $Header.HeaderCRC32)
		CalcCRC32   = $calc
	}
}

# --- Partition Entries ---------------------------------------------------

function Read-PartitionEntries {
	param(
		[System.IO.FileStream]$Stream,
		[pscustomobject]$Header,
		[int]$SectorSize
	)

	$totalBytes = $Header.NumEntries * $Header.EntrySize
	$offset = $Header.PartEntriesLBA * $SectorSize
	$buf = Read-Bytes -Stream $Stream -Offset $offset -Count $totalBytes
	return $buf
}

function Validate-BackupPartitionEntries {
	param(
		[System.IO.FileStream]$Stream,
		[pscustomobject]$BackupHeader,
		[int]$SectorSize
	)

	$totalBytes = $BackupHeader.NumEntries * $BackupHeader.EntrySize
	$offset = $BackupHeader.PartEntriesLBA * $SectorSize

	$entries = Read-Bytes -Stream $Stream -Offset $offset -Count $totalBytes

	$calc = Compute-CRC32 $entries
	$crcOK = ($calc -eq $BackupHeader.EntriesCRC32)

	[pscustomobject]@{
		CRC32OK   = $crcOK
		CalcCRC32 = $calc
		StoredCRC = $BackupHeader.EntriesCRC32
		Entries   = $entries
	}
}
function Validate-PartitionEntries {
	param(
		[byte[]]$Entries,
		[pscustomobject]$Header
	)

	$calc = Compute-CRC32 $Entries
	$crcOK = ($calc -eq $Header.EntriesCRC32)

	$list = @()
	for ($i = 0; $i -lt $Header.NumEntries; $i++) {
		$off = $i * $Header.EntrySize
		$typeGuid = Format-GuidLE $Entries $off
		$uniqGuid = Format-GuidLE $Entries ($off + 16)
		$firstLBA = Get-UInt64LE $Entries ($off + 32)
		$lastLBA  = Get-UInt64LE $Entries ($off + 40)
		$attrs    = Get-UInt64LE $Entries ($off + 48)
		$nameBytes = $Entries[($off + 56)..($off + $Header.EntrySize - 1)]
		$name = -join ($nameBytes | Where-Object { $_ -ne 0 } | ForEach-Object {[char]$_})

		if ($typeGuid -ne [Guid]::Empty) {
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
	}

	[pscustomobject]@{
		CRC32OK   = $crcOK
		CalcCRC32 = $calc
		Entries   = $list
	}
}

# --- Filesystem Probing --------------------------------------------------

function Probe-Filesystem {
	param(
		[System.IO.FileStream]$Stream,
		[int64]$StartLBA,
		[int]$SectorSize
	)

	$boot = Read-Bytes -Stream $Stream -Offset ($StartLBA * $SectorSize) -Count $SectorSize

	# FAT12/16/32: "FAT" at 0x36 or 0x52, but easiest is OEM + jump
	$oem = [System.Text.Encoding]::ASCII.GetString($boot, 3, 8).Trim()

	$fatSig1 = [System.Text.Encoding]::ASCII.GetString($boot, 54, 8).Trim()
	$fatSig2 = [System.Text.Encoding]::ASCII.GetString($boot, 82, 8).Trim()

	$ntfs = [System.Text.Encoding]::ASCII.GetString($boot, 3, 4)

	if ($ntfs -eq 'NTFS') {
		return 'NTFS'
	} elseif ($fatSig1 -like 'FAT*' -or $fatSig2 -like 'FAT*') {
		return 'FAT'
	} elseif ($oem -like 'MSDOS*' -or $oem -like 'MSWIN*') {
		return 'FAT-like'
	} else {
		return 'Unknown'
	}
}

function Fix-GptHeaderCRC {
	param(
		[System.IO.FileStream]$Stream,
		[pscustomobject]$Header,
		[int]$SectorSize
	)

	Write-Host "  → Fixing GPT Header CRC32..."

	# Read raw header
	$offset = $Header.CurrentLBA * $SectorSize
	$hdr = Read-Bytes -Stream $Stream -Offset $offset -Count $SectorSize

	# Zero CRC field
	$hdr[16] = 0
	$hdr[17] = 0
	$hdr[18] = 0
	$hdr[19] = 0

	# Compute correct CRC
	$calc = Compute-CRC32 $hdr[0..($Header.HeaderSize - 1)]
	Write-Host ("    Correct CRC32 = 0x{0:X8}" -f $calc)

	# Write CRC back into header
	[BitConverter]::GetBytes([uint32]$calc).CopyTo($hdr, 16)

	# Write header back to disk
	$Stream.Seek($offset, 'Begin') | Out-Null
	$Stream.Write($hdr, 0, $hdr.Length)

	Write-Host "    Header CRC32 repaired."
}

function Fix-GptEntriesCRC {
	param(
		[System.IO.FileStream]$Stream,
		[pscustomobject]$Header,
		[int]$SectorSize
	)

	Write-Host "  → Fixing GPT Partition Entries CRC32..."

	# Read entries
	$entries = Read-PartitionEntries -Stream $Stream -Header $Header -SectorSize $SectorSize

	# Compute correct CRC
	$calc = Compute-CRC32 $entries
	Write-Host ("    Correct Entries CRC32 = 0x{0:X8}" -f $calc)

	# Read header
	$hdrOffset = $Header.CurrentLBA * $SectorSize
	$hdr = Read-Bytes -Stream $Stream -Offset $hdrOffset -Count $SectorSize

	# Write new CRC into header
	[BitConverter]::GetBytes([uint32]$calc).CopyTo($hdr, 88)

	# Zero header CRC before recomputing header CRC
	$hdr[16] = 0
	$hdr[17] = 0
	$hdr[18] = 0
	$hdr[19] = 0

	# Recompute header CRC
	$newHdrCRC = Compute-CRC32 $hdr[0..($Header.HeaderSize - 1)]
	Write-Host ("    New Header CRC32 = 0x{0:X8}" -f $newHdrCRC)

	# Write header CRC
	[BitConverter]::GetBytes([uint32]$newHdrCRC).CopyTo($hdr, 16)

	# Write header back
	$Stream.Seek($hdrOffset, 'Begin') | Out-Null
	$Stream.Write($hdr, 0, $hdr.Length)

	Write-Host "    Partition Entries CRC32 repaired."
}

# --- FAT16/FAT32 parsing + walking --------------------------------------

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

	$fatSize = if ($fatSize16 -ne 0) { $fatSize16 } else { $fatSize32 }
	$totalSectors = if ($totalSectors16 -ne 0) { $totalSectors16 } else { $totalSectors32 }

	$rootDirSectors = [math]::Ceiling(($rootEntryCount * 32) / $bytesPerSector)
	$fatType = if ($rootEntryCount -ne 0) { "FAT16" } else { "FAT32" }

	return [pscustomobject]@{
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
	return Read-Bytes -Stream $Stream -Offset ($FatLBA * $SectorSize) -Count $bytes
}

function Get-FAT16NextCluster {
	param(
		[byte[]]$FAT,
		[int]$Cluster
	)
	$offset = $Cluster * 2
	$val = [BitConverter]::ToUInt16($FAT, $offset)
	if ($val -ge 0xFFF8) { return -1 }
	return $val
}

function Get-FAT32NextCluster {
	param(
		[byte[]]$FAT,
		[int]$Cluster
	)
	$offset = $Cluster * 4
	$val = [BitConverter]::ToUInt32($FAT, $offset) -band 0x0FFFFFFF
	if ($val -ge 0x0FFFFFF8) { return -1 }
	return $val
}

function Read-FAT32Directory {
	param(
		[System.IO.FileStream]$Stream,
		[byte[]]$FAT,
		[int64]$DataRegionLBA,
		[int]$SectorSize,
		[int]$SectorsPerCluster,
		[int]$Cluster,
		[string]$Path = "",
		[System.Collections.Generic.HashSet[int]]$Visited = $(New-Object 'System.Collections.Generic.HashSet[int]'),
		[UInt32]$Depth
	)

	if($Visited.Contains($Cluster)){return}
	$Visited.Add($Cluster) | Out-Null

	$clusterSizeBytes = $SectorsPerCluster * $SectorSize

	while($Cluster -ne -1){
		$clusterLBA = $DataRegionLBA + (($Cluster - 2) * $SectorsPerCluster)
		$buf = Read-Bytes -Stream $Stream -Offset ($clusterLBA * $SectorSize) -Count $clusterSizeBytes

		for($i = 0; $i -lt $clusterSizeBytes; $i += 32){
			$entry = $buf[$i..($i+31)]

			if($entry[0] -eq 0x00){return}
			if($entry[0] -eq 0xE5){continue}

			$attr = $entry[11]
			if($attr -band 0x0F -eq 0x0F){continue}

			$name = ([System.Text.Encoding]::ASCII.GetString($entry, 0, 11)).Trim()
			$firstCluster = ([BitConverter]::ToUInt16($entry, 20) -shl 16) -bor [BitConverter]::ToUInt16($entry, 26)
			$size = [BitConverter]::ToUInt32($entry, 28)
			Write-Host ("DEBUG ENTRY: {0} firstCluster={1}" -f $name, $firstCluster) -ForegroundColor Blue

			$fullPath = if ($Path -eq "") { $name } else { "$Path/$name" }

			if ($attr -band 0x10) {
				$out = "DIR "
				for([uint]$i_ = 0; $i_ -lt $Depth; $i_++){$out += "`t"}
				$out += "$name/"
				Write-Host $out
				if ($name -ne "." -and $name -ne ".." -and $firstCluster -ge 2) {
					Read-FAT32Directory -Stream $Stream -FAT $FAT `
						-DataRegionLBA $DataRegionLBA -SectorSize $SectorSize `
						-SectorsPerCluster $SectorsPerCluster `
						-Cluster $firstCluster -Path $fullPath -Visited $Visited -Depth ($Depth + 1)
				}
			}else{
				$out = "FILE "
				for([uint]$i_ = 0; $i_ -lt $Depth; $i_++){$out += "`t"}
				$out += ("$($name -replace " ",'.')  ({0} bytes)" -f $size)
				Write-Host $out
			}
		}

		$Cluster = Get-FAT32NextCluster -FAT $FAT -Cluster $Cluster
	}
}

# function Fix-GptBackupHeader {
#     param(
#         [System.IO.FileStream]$Stream,
#         [pscustomobject]$Primary,
#         [byte[]]$Entries,
#         [int]$SectorSize
#     )

#     Write-Host "  → Fixing Backup GPT Header..."

#     $backupLBA = $Primary.BackupLBA
#     $backupOffset = $backupLBA * $SectorSize

#     # Read existing backup header
#     $hdr = Read-Bytes -Stream $Stream -Offset $backupOffset -Count $SectorSize

#     # Fix required fields
#     [BitConverter]::GetBytes([uint64]$backupLBA).CopyTo($hdr, 24)  # CurrentLBA
#     [BitConverter]::GetBytes([uint64]1).CopyTo($hdr, 32)           # BackupLBA

#     # Backup partition entries LBA = backupLBA - (NumEntries * EntrySize / SectorSize)
#     $entriesSectors = [int]($Primary.NumEntries * $Primary.EntrySize / $SectorSize)
#     $backupEntriesLBA = $backupLBA - $entriesSectors
#     [BitConverter]::GetBytes([uint64]$backupEntriesLBA).CopyTo($hdr, 72)

#     # Fix entries CRC
#     $entriesCRC = Compute-CRC32 $Entries
#     [BitConverter]::GetBytes([uint32]$entriesCRC).CopyTo($hdr, 88)

#     # Zero header CRC before recomputing
#     $hdr[16] = 0
#     $hdr[17] = 0
#     $hdr[18] = 0
#     $hdr[19] = 0

#     # Compute backup header CRC
#     $newCRC = Compute-CRC32 $hdr[0..($Primary.HeaderSize - 1)]
#     [BitConverter]::GetBytes([uint32]$newCRC).CopyTo($hdr, 16)

#     # Write back
#     $Stream.Seek($backupOffset, 'Begin') | Out-Null
#     $Stream.Write($hdr, 0, $hdr.Length)

#     Write-Host ("    Backup GPT header repaired (CRC=0x{0:X8})." -f $newCRC)
# }
function Fix-GptBackupHeader {
    param(
        [System.IO.FileStream]$Stream,
        [pscustomobject]$Primary,      # Object parsed from Primary Header
        [byte[]]$PrimaryEntries,      # The raw bytes of the Primary Partition Table
        [int]$SectorSize
    )

    Write-Host "  → Validating and Fixing Backup GPT Infrastructure..." -ForegroundColor Cyan

    $backupLBA = $Primary.BackupLBA
    $backupOffset = $backupLBA * $SectorSize
    
    # Calculate sectors for entries safely
    $entriesSectors = [int][Math]::Ceiling(($Primary.NumEntries * $Primary.EntrySize) / $SectorSize)
    $backupEntriesLBA = $backupLBA - $entriesSectors
    $backupEntriesOffset = $backupEntriesLBA * $SectorSize

    # 1. VALIDATE/REPAIR PARTITION ENTRIES FIRST
    $currentBackupEntries = Read-Bytes -Stream $Stream -Offset $backupEntriesOffset -Count ($Primary.NumEntries * $Primary.EntrySize)
    
    # Check if backup matches primary exactly (fixes the "Tables Differ" gdisk error)
    if ([System.BitConverter]::ToString($currentBackupEntries) -ne [System.BitConverter]::ToString($PrimaryEntries)) {
        Write-Host "    [!] Backup Table mismatch/corrupt. Synchronizing with Primary..." -ForegroundColor Yellow
        $Stream.Seek($backupEntriesOffset, 'Begin') | Out-Null
        $Stream.Write($PrimaryEntries, 0, $PrimaryEntries.Length)
        $finalEntriesCRC = Compute-CRC32 $PrimaryEntries
    } else {
        $finalEntriesCRC = Compute-CRC32 $currentBackupEntries
    }

    # 2. CONSTRUCT/FIX BACKUP HEADER
    # We create a fresh buffer to ensure we don't carry over "ghost" corruption
    $hdr = New-Object byte[] $SectorSize 

    # Fill mandatory static fields (Fixes the "Header Size Corrupted" issue)
    [System.Text.Encoding]::ASCII.GetBytes("EFI PART").CopyTo($hdr, 0)
    [BitConverter]::GetBytes([uint32]$Primary.Revision).CopyTo($hdr, 8)
    [BitConverter]::GetBytes([uint32]$Primary.HeaderSize).CopyTo($hdr, 12) # <--- CRITICAL FIX
    $Primary.DiskGuid.ToByteArray().CopyTo($hdr, 56)
    [BitConverter]::GetBytes([uint32]$Primary.NumEntries).CopyTo($hdr, 80)
    [BitConverter]::GetBytes([uint32]$Primary.EntrySize).CopyTo($hdr, 84)

    # Fill dynamic LBA fields
    [BitConverter]::GetBytes([uint64]$backupLBA).CopyTo($hdr, 24)        # CurrentLBA
    [BitConverter]::GetBytes([uint64]1).CopyTo($hdr, 32)                 # BackupLBA (points to Primary)
    [BitConverter]::GetBytes([uint64]$Primary.FirstUsableLBA).CopyTo($hdr, 40)
    [BitConverter]::GetBytes([uint64]$Primary.LastUsableLBA).CopyTo($hdr, 48)
    [BitConverter]::GetBytes([uint64]$backupEntriesLBA).CopyTo($hdr, 72)
    [BitConverter]::GetBytes([uint32]$finalEntriesCRC).CopyTo($hdr, 88)

    # Compute Header CRC
    $hdr[16] = 0;	$hdr[17] = 0;	$hdr[18] = 0;	$hdr[19] = 0;
    $newCRC = Compute-CRC32 $hdr[0..($Primary.HeaderSize - 1)]
    [BitConverter]::GetBytes([uint32]$newCRC).CopyTo($hdr, 16)

    # Write repaired header back to disk
    $Stream.Seek($backupOffset, 'Begin') | Out-Null
    $Stream.Write($hdr, 0, $hdr.Length)

    Write-Host ("    [OK] Backup GPT repaired (Header CRC=0x{0:X8})." -f $newCRC) -ForegroundColor Green
}

# function Repair-GPT {
#     param(
#         [System.IO.FileStream]$Stream,
#         [pscustomobject]$Header,
#         [pscustomobject]$HeaderVal,
#         [pscustomobject]$EntriesVal,
#         [int]$SectorSize
#     )

#     $needFix = $false

#     if (-not $HeaderVal.CRC32OK) {
#         Write-Host "⚠ GPT Header CRC32 is invalid."
#         Fix-GptHeaderCRC -Stream $Stream -Header $Header -SectorSize $SectorSize
#         $needFix = $true
#     }

#     if (-not $EntriesVal.CRC32OK) {
#         Write-Host "⚠ GPT Entries CRC32 is invalid."
#         Fix-GptEntriesCRC -Stream $Stream -Header $Header -SectorSize $SectorSize
#         $needFix = $true
#     }

#     if ($needFix) {
#         Write-Host "✔ GPT successfully repaired."
#     } else {
#         Write-Host "✔ GPT is already valid."
#     }
# }
function Repair-GPT {
	param(
		[System.IO.FileStream]$Stream,
		[pscustomobject]$Header,
		[pscustomobject]$HeaderVal,
		[pscustomobject]$EntriesVal,
		[byte[]]$Entries,
		[int]$SectorSize
	)

	$needFix = $false

	if (-not $HeaderVal.CRC32OK) {
		Write-Host "⚠ GPT Header CRC32 is invalid."
		Fix-GptHeaderCRC -Stream $Stream -Header $Header -SectorSize $SectorSize
		$needFix = $true
	}

	if (-not $EntriesVal.CRC32OK) {
		Write-Host "⚠ GPT Entries CRC32 is invalid."
		Fix-GptEntriesCRC -Stream $Stream -Header $Header -SectorSize $SectorSize
		$needFix = $true
	}

	if ($needFix) {
		Fix-GptBackupHeader -Stream $Stream -Primary $Header -PrimaryEntries $Entries -SectorSize $SectorSize
		Write-Host "✔ GPT successfully repaired."
	} else {
		Write-Host "✔ GPT is already valid."
	}
}


# --- Main ---------------------------------------------------------------

$fs = [System.IO.File]::Open($ImagePath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::Read)
try {
	$length = $fs.Length
	$totalLBAs = [int64]($length / $SectorSize)

	Write-Host "Image: $ImagePath"
	Write-Host "Size : $length bytes ($totalLBAs sectors of $SectorSize bytes)"
	Write-Host ""

	# MBR
	$mbr = Read-Bytes -Stream $fs -Offset 0 -Count $SectorSize
	$mbrInfo = Validate-MBR $mbr
	Write-Host "MBR:"
	Write-Host "  Signature OK     : $($mbrInfo.SignatureOK)"
	Write-Host "  Protective Found : $($mbrInfo.ProtectiveFound)"
	if(-not ($mbrInfo.SignatureOK -and $mbrInfo.ProtectiveFound)){throw ''}
	foreach($p in $mbrInfo.Partitions){
		Write-Host ("  Part{0}: Type={1} LBA={2} Sectors={3}" -f $p.Index, ('0x{0:X2}' -f $p.Type), $p.LbaStart, $p.Sectors)
	}
	Write-Host ""

	# Primary GPT
	$gptSector = Read-Bytes -Stream $fs -Offset $SectorSize -Count $SectorSize
	$gpt = Parse-GptHeader $gptSector
	$gptVal = Validate-GptHeader $gpt

	Write-Host "Primary GPT Header:"
	Write-Host "  Signature        : $($gpt.Signature) (OK=$($gptVal.SignatureOK))"
	Write-Host "  HeaderSize       : $($gpt.HeaderSize) (OK=$($gptVal.SizeOK))"
	Write-Host ("  HeaderCRC32 OK   : $($gptVal.CRC32OK) (Calc=0x{0:X8}, Stored=0x{1:X8})" -f $gptVal.CalcCRC32, $gpt.HeaderCRC32)
	Write-Host "  CurrentLBA       : $($gpt.CurrentLBA)"
	Write-Host "  BackupLBA        : $($gpt.BackupLBA)"
	Write-Host "  FirstUsableLBA   : $($gpt.FirstUsableLBA)"
	Write-Host "  LastUsableLBA    : $($gpt.LastUsableLBA)"
	Write-Host "  Disk GUID        : $($gpt.DiskGuid)"
	Write-Host "  PartEntriesLBA   : $($gpt.PartEntriesLBA)"
	Write-Host "  NumEntries       : $($gpt.NumEntries)"
	Write-Host "  EntrySize        : $($gpt.EntrySize)"
	Write-Host ("  EntriesCRC32     : 0x{0:X8}" -f $gpt.EntriesCRC32)
	Write-Host ""

	# Partition entries
	$entriesBuf = Read-PartitionEntries -Stream $fs -Header $gpt -SectorSize $SectorSize
	$entriesInfo = Validate-PartitionEntries -Entries $entriesBuf -Header $gpt

	Write-Host "Partition Entries:"
	Write-Host ("  CRC32 OK         : $($entriesInfo.CRC32OK) (Calc=0x{0:X8}, Stored=0x{1:X8})" -f $entriesInfo.CalcCRC32, $gpt.EntriesCRC32)
	foreach ($e in $entriesInfo.Entries) {
		Write-Host ("  [{0}] {1}  LBA={2}-{3}  Name='{4}'" -f $e.Index, $e.TypeGuid, $e.FirstLBA, $e.LastLBA, $e.Name)
	}
	Write-Host ""

	# Backup GPT
	$backupHdrOffset = $gpt.BackupLBA * $SectorSize
	$backupSector = Read-Bytes -Stream $fs -Offset $backupHdrOffset -Count $SectorSize
	$gptBackup = Parse-GptHeader $backupSector
	$gptBackupVal = Validate-GptHeader $gptBackup

	Write-Host "Backup GPT Header:"
	Write-Host "  Signature        : $($gptBackup.Signature) (OK=$($gptBackupVal.SignatureOK))"
	Write-Host "  HeaderSize       : $($gptBackup.HeaderSize) (OK=$($gptBackupVal.SizeOK))"
	Write-Host ("  HeaderCRC32 OK   : $($gptBackupVal.CRC32OK) (Calc=0x{0:X8}, Stored=0x{1:X8})" -f $gptBackupVal.CalcCRC32, $gptBackup.HeaderCRC32)
	Write-Host "  CurrentLBA       : $($gptBackup.CurrentLBA)"
	Write-Host "  BackupLBA        : $($gptBackup.BackupLBA)"
	Write-Host "  FirstUsableLBA   : $($gptBackup.FirstUsableLBA)"
	Write-Host "  LastUsableLBA    : $($gptBackup.LastUsableLBA)"
	Write-Host "  Disk GUID        : $($gptBackup.DiskGuid)"
	Write-Host "  PartEntriesLBA   : $($gptBackup.PartEntriesLBA)"
	Write-Host "  NumEntries       : $($gptBackup.NumEntries)"
	Write-Host "  EntrySize        : $($gptBackup.EntrySize)"
	Write-Host ("  EntriesCRC32     : 0x{0:X8}" -f $gptBackup.EntriesCRC32)
	Write-Host ""
	Write-Host "Backup GPT Header:"
	Write-Host "  Signature        : $($gptBackup.Signature) (OK=$($gptBackupVal.SignatureOK))"
	Write-Host "  HeaderCRC32 OK   : $($gptBackupVal.CRC32OK)"
	Write-Host "  CurrentLBA       : $($gptBackup.CurrentLBA)"
	Write-Host "  BackupLBA        : $($gptBackup.BackupLBA)"
	Write-Host ""

	# --- Backup Partition Entries ---
	$backupEntriesInfo = Validate-BackupPartitionEntries -Stream $fs -BackupHeader $gptBackup -SectorSize $SectorSize

	Write-Host "Backup Partition Entries:"
	Write-Host ("  CRC32 OK         : {0} (Calc=0x{1:X8}, Stored=0x{2:X8})" -f $backupEntriesInfo.CRC32OK, $backupEntriesInfo.CalcCRC32, $backupEntriesInfo.StoredCRC)
	Write-Host ""



	# Repair-GPT -Stream $fs -Header $gpt -HeaderVal $gptVal -EntriesVal $entriesInfo -SectorSize $SectorSize
	Repair-GPT -Stream $fs -Header $gpt -HeaderVal $gptVal -EntriesVal $entriesInfo -Entries $entriesBuf -SectorSize $SectorSize

	# Filesystems
	# Write-Host "Filesystem Probes:"
	# foreach ($e in $entriesInfo.Entries) {
	#     $fsType = Probe-Filesystem -Stream $fs -StartLBA $e.FirstLBA -SectorSize $SectorSize
	#     Write-Host ("  [{0}] LBA={1}-{2} Name='{3}' FS={4}" -f $e.Index, $e.FirstLBA, $e.LastLBA, $e.Name, $fsType)
	# }
	Write-Host "Filesystem Probes:"
	foreach ($e in $entriesInfo.Entries) {

		# Detect filesystem type
		$fsType = Probe-Filesystem -Stream $fs -StartLBA $e.FirstLBA -SectorSize $SectorSize
		Write-Host ("  [{0}] LBA={1}-{2} Name='{3}' FS={4}" -f $e.Index, $e.FirstLBA, $e.LastLBA, $e.Name, $fsType)

		# Only handle FAT16/FAT32
		if ($fsType -eq "FAT" -or $fsType -eq "FAT-like") {

			# Read boot sector
			$boot = Read-Bytes -Stream $fs -Offset ($e.FirstLBA * $SectorSize) -Count $SectorSize
			$fatBoot = Parse-FATBootSector $boot

			# Load FAT table
			$fatLBA = $e.FirstLBA + $fatBoot.ReservedSectors
			$FAT = Read-FAT -Stream $fs -FatLBA $fatLBA -FatSize $fatBoot.FATSize -SectorSize $SectorSize

			#
			# ---------------- FAT32 ----------------
			#
			if ($fatBoot.Type -eq "FAT32") {

				$dataRegionLBA = $e.FirstLBA + $fatBoot.ReservedSectors + ($fatBoot.FATCount * $fatBoot.FATSize)

				Write-Host "    FAT32 Root Directory:"
				Write-Host ("    DEBUG: RootCluster={0} DataRegionLBA={1} SectorsPerCluster={2}" -f $fatBoot.RootCluster, $dataRegionLBA, $fatBoot.SectorsPerCluster)

				Read-FAT32Directory -Stream $fs -FAT $FAT `
					-DataRegionLBA $dataRegionLBA -SectorSize $SectorSize `
					-SectorsPerCluster $fatBoot.SectorsPerCluster `
					-Cluster $fatBoot.RootCluster -Path ""
			}

			#
			# ---------------- FAT16 ----------------
			#
			elseif ($fatBoot.Type -eq "FAT16") {
				$rootDirLBA = $e.FirstLBA + $fatBoot.ReservedSectors + ($fatBoot.FATCount * $fatBoot.FATSize)
				$dataRegionLBA = $rootDirLBA + $fatBoot.RootDirSectors

				Write-Host "    FAT16 Root Directory:"

				# FAT16 root directory is NOT cluster-based — it's a fixed region
				for ($s = 0; $s -lt $fatBoot.RootDirSectors; $s++) {

					# Treat each sector as a pseudo-cluster index
					$pseudoCluster = 2 + $s

					Read-FAT32Directory -Stream $fs -FAT $FAT `
						-DataRegionLBA $dataRegionLBA -SectorSize $SectorSize `
						-SectorsPerCluster 1 `
						-Cluster $pseudoCluster -Path ""
				}
			}
		}
	}

} finally {
	$fs.Dispose()
}
