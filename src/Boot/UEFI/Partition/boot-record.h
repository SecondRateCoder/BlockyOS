#pragma once

#include "Boot/UEFI/standard.h"

#define GPTBlockCount 1024

#define NULLSTR(s) (char[s]){0}
#define defenum(type, name) typedef type name; enum

typedef size_t LBA;
typedef uint32_t CRC32hash;
typedef PACKEDSTRUCT size_t GUID[2];

typedef struct GPTPartitionAttributes{
    size_t RequiredPartition      : 1;
    size_t NoBlockIOProtocol      : 1;
    size_t LegacyBIOSBootable     : 1;
    size_t UEFIReserved           : 45;
    size_t LinuxFlags             : 8;
    size_t VendorFlags            : 4;
    size_t MicrosoftFlags         : 4;
}GPTPartitionAttributes;
defenum(GUID, GUIDTypes){
    GUID_ESPHIGH = 0xC12A7328F81F11D2,
    GUID_ESPLOW = 0xBA4B00A0C93EC93B
};
defenum(uint8_t, OSType){
    GPTProtective = 0xEE,
    NoBootPartition = 0x00,
    BootablePartition = 0x01
};
typedef struct CHS{
    uint8_t cylinder,
            head,
            sector;
}PACKEDSTRUCT CHS;

typedef struct LBA0HEADER{
    uint8_t bootIndicator   : 1;
    CHS startingCHS;
    uint8_t Type;
    CHS endingCHS;
    LBA startingLBA;
    LBA endingLBA;
}PACKEDSTRUCT LBA0HEADER;
typedef struct LBA0{
    LBA0HEADER header;
    uint8_t reserved[GPTBlockCount - sizeof(LBA0HEADER)];
}PACKEDSTRUCT LBA0;

typedef struct LBA1HEADER{
    char signature[8];
    uint32_t GPTrevision,
             headerSize;
    CRC32hash CRC32Checksum;
    uint32_t reserved;
    LBA localLBA,
        duplicateLBA,
        firstUsable,
        lastUsable;
    GUID diskGUID;
    LBA GUIDPartitionEntryArray;
    uint32_t PartitionEntryCount;
    uint32_t PartitionEntrySize;
    CRC32hash CRC32PartitionHash;
}PACKEDSTRUCT LBA1HEADER;
typedef struct LBA1{
    LBA1HEADER header;
    uint8_t reserved[GPTBlockCount - sizeof(LBA1HEADER)];
}PACKEDSTRUCT LBA1;

typedef struct PartitionEntry{
    GUID TypeGUID,
         UniqueGUID;
    LBA Base,
        Limit;
    GPTPartitionAttributes attributes;
    wchar_t Name[72 / sizeof(wchar_t)];
}PACKEDSTRUCT PartitionEntry;
typedef PartitionEntry LBA2[GPTBlockCount / sizeof(PartitionEntry)];

typedef struct GPTUEFIBootRecord{
    LBA0 lba0;
    LBA1 lba1;
    LBA2 lba2;
}PACKEDSTRUCT GPTUEFIBootRecord;

extern const uint8_t KernelSize, DriveSize, LegacyKernelSize;