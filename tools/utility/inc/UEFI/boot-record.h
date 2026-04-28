#include <efi.h>
#include "tools/utility/util.h"

#define GPTBlockCount 1024
typedef size_t LBA;
typedef uint32_t CRC32hash;
typedef size_t GUID[2];

defenum(size_t, PartitionAttributes){
    BOOTABLEPARTITION = 0x2
};
defenum(GUID, GUIDTypes){
    GUID_ESP = 0xC12A7328F81F11D2BA4B00A0C93EC93B
};
defenum(uint8_t, OSType){
    GPTProtective = 0xEE
};
typedef struct CHS{
    uint8_t cylinder,
            head,
            sector;
}PACKEDSTRUCT CHS;

typedef struct LBA0HEADER{
    uint8_t bootIndicator;
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
    uint32_t Partitioncount;
    uint32_t PartitionEntrySize;
    CRC32hash CRC32PartitionHash
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
    PartitionAttributes attributes;
    CHAR16 Name[72 / sizeof(CHAR16)];
}PACKEDSTRUCT PartitionEntry;
typedef PartitionEntry LBA2[GPTBlockCount / sizeof(PartitionEntry)];

typedef struct GPTUEFIBootRecord{
    LBA0 lba0;
    LBA1 lba1;
    LBA2 lba2;
}PACKEDSTRUCT GPTUEFIBootRecord;