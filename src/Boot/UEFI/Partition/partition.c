#include "boot-record.h"

static GPTUEFIBootRecord LINKERSECTION("UEFIPartitionTable") PartitionTable = {
    .lba0 = {
        .header = {
            .bootIndicator = BootablePartition,
            .startingCHS = {
                .cylinder = 0x00,
                .head = 0x02,
                .sector = 0x00
            },
            .Type = GPTProtective,
            .endingCHS = {
                .cylinder = 0xFF,
                .head = 0xFF,
                .sector = 0xFF
            },
            .startingLBA = 0x01,
            .endingLBA = (size_t)(&DriveSize - 1)
        },
        .reserved = {0}
    },
    .lba1 = {
        .header = {
            .signature = {0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54},
            .GPTrevision = 0x10000,
            .headerSize = sizeof(LBA1HEADER),
            .CRC32Checksum = 0x8CC0C9C3,
            .localLBA = 2,
            .duplicateLBA = (((1024 * 1024) / 512) + 3) + 1,
            .firstUsable = 3,
            .lastUsable = (size_t)(&DriveSize - 1),
            .diskGUID = 0,
            .GUIDPartitionEntryArray = 2,
            .PartitionEntryCount = 2,
            .PartitionEntrySize = sizeof(PartitionEntry),
            .CRC32PartitionHash = 0
        },
        .reserved = {0}
    },
    .lba2 = {
        // Boot Partition
        {
            .TypeGUID = {GUID_ESPHIGH, GUID_ESPLOW},
            .UniqueGUID = {0},
            .Base = 3,
            .Limit = (((1024 * 1024) / 512) + 3),
            .attributes = {
                .RequiredPartition = 1,
                .LegacyBIOSBootable = 0,
                .NoBlockIOProtocol = 0
            },
            .Name = L"Flash #0"
        },
        // Legacy Boot Partition
        {
            .TypeGUID = {GUID_ESPHIGH, GUID_ESPLOW},
            .UniqueGUID = {0},
            .Base = (((1024 * 1024) / 512) + 4),
            .Limit = (size_t)(&LegacyKernelSize),
            .attributes = {
                .RequiredPartition = 1,
                .LegacyBIOSBootable = 1,
                .NoBlockIOProtocol = 0
            },
            .Name = L"Flash #0"
        },
        // Remaining Size of Drive
        {
            .TypeGUID = {0, 1},
            .UniqueGUID = {0},
            .Base = (((1024 * 1024) / 512) + 4) + ((size_t)(&LegacyKernelSize)),
            .Limit = (size_t)(&DriveSize),
            .attributes = {
                .RequiredPartition = 0,
                .LegacyBIOSBootable = 0,
                .NoBlockIOProtocol = 0
            },
            .Name = L"Drive #0"
        },
    }
};

void main(){return;}