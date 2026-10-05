#pragma once

#include "kernel/services/IO/service.h"
#include "kernel/libcrt/hardware/IO/IO.h"
#include "kernel/libcrt/hardware/IDT/APIC/IOAPIC.h"
#include "kernel/libcrt/hardware/PIT.h"

#define LegacySectorSize	512

#define ATAPIPrimaryCommandPort	0x1F0
#define ATAPIPrimaryControlPort	(ATAPIPrimaryCommandPort + ATAPI_CONTROL)

#define ATAPISecondaryCommandPort	0x170
#define ATAPISecondaryControlPort	(ATAPISecondaryCommandPort + ATAPI_CONTROL)

#define ATAPI_DATA 0
#define ATAPI_ERROR_R 1
#define ATAPI_SECTOR_COUNT 2
#define ATAPI_LBA_LOW 3
#define ATAPI_LBA_MID 4
#define ATAPI_LBA_HIGH 5
#define ATAPI_DRIVE_SELECT 6
#define ATAPI_COMMAND_REGISTER 7
#define ATAPI_REG_HDDEVSEL 0x06

// Control register defines
#define ATAPI_CONTROL 0x206

#define ATAPI_ALTERNATE_STATUS 0

enumdef(uint8_t, ATAPICommandByte){
	ATAPI_TEST_UNIT_READY = 0x00, 
	ATAPI_REQUEST_SENSE = 0x03, 
	ATAPI_FORMAT_UNIT = 0x04, 
	ATAPI_INQUIRY = 0x12, 
	ATAPI_START_UNIT = 0x1B, ATAPI_STOP_UNIT = ATAPI_START_UNIT, ATAPI_EJECT_DISK = ATAPI_STOP_UNIT, 
	ATAPI_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E, 
	ATAPI_READ_FORMAT_CAPACITIES = 0x23, 
	ATAPI_READ_CAPACITY = 0x25, 
	ATAPI_READ10 = 0x28, 
	ATAPI_WRITE10 = 0x2A, 
	ATAPI_SEEK10 = 0x2B, 
	ATAPI_WRITE_AND_VERIFY10 = 0x2E, 
	ATAPI_VERIFY = 0x2F, 
	ATAPI_SYNCHRONIZE_CACHE = 0x35, 
	ATAPI_WRITE_BUFFER = 0x3B, 
	ATAPI_READ_BUFFER = 0x3C, 
	ATAPI_READ_TOC = 0x43, ATAPI_READ_PMA = ATAPI_READ_TOC, ATAPI_READ_ATIP = ATAPI_READ_PMA, 
	ATAPI_GET_CONFIGURATION = 0x46, 
	ATAPI_GET_EVENT_STATUS_NOTIFICATION = 0x4A, 
	ATAPI_READ_DISC_INFORMATION = 0x51, 
	ATAPI_READ_TRACK_INFORMATION = 0x52, 
	ATAPI_RESERVE_TRACK = 0x53, 
	ATAPI_SEND_OPC_INFORMATION = 0x54, 
	ATAPI_MODE_SELECT10 = 0x55, 
	ATAPI_REPAIR_TRACK = 0x58, 
	ATAPI_MODE_SENSE10 = 0x5A, 
	ATAPI_CLOSE_TRACK_SESSION = 0x5B, 
	ATAPI_READ_BUFFER_CAPACITY = 0x5C, 
	ATAPI_SEND_CUE_SHEET = 0x5D, 
	ATAPI_REPORT_LUNS = 0xA0, 
	ATAPI_BLANK = 0xA1, 
	ATAPI_SECURITY_PROTOCOL_IN = 0xA2, 
	ATAPI_SEND_KEY = 0xA3, 
	ATAPI_REPORT_KEY = 0xA4, 
	ATAPI_LOAD_UNLOAD_MEDIUM = 0xA6, 
	ATAPI_SET_AHEAD = 0xA7, ATAPI_READ_AHEAD = ATAPI_SET_AHEAD, 
	ATAPI_READ12 = 0xA8, 
	ATAPI_WRITE12 = 0xAA, 
	ATAPI_READ_MEDIA_SERIAL_NUMBER = 0xAB, ATAPI_SERVICE_ACTION_IN = 0x01, 
	ATAPI_GET_PERFORMANCE = 0xAC, 
	ATAPI_READ_DISC_STRUCTURE = 0xAD, 
	ATAPI_SECURITY_PROTOCOL_OUT = 0xB5, 
	ATAPI_SET_STREAMING = 0xB6, 
	ATAPI_READ_CD_MSF = 0xB9, 
	ATAPI_SET_CD_SPEED = 0xBB, 
	ATAPI_MECHANISM_STATUS = 0xBD, 
	ATAPI_READ_CD = 0xBE, 
	ATAPI_SEND_DISC_STRUCTURE = 0xBF, 
};

typedef volatile struct{
	ATAPICommandByte    Command;
	uint8_t             Payload[11];
}__packed ATAPICommand;
typedef struct{
	//	1: Native, 0: Compatibility
	uint8_t		PrimaryChannelMode		: 1;
	//	1: Native, 0: Compatibility
	uint8_t		SecondaryChannelMode	: 1;
	uint8_t								: 4;
	uint8_t		BusMasterDMASupported	: 1;
}__packed	ATAPI_PCIProgIF;
typedef struct {
	// Must be 32-bit aligned
	uint32_t	PhysicalAddress; 
	// Note: A value of 0 indicates 65,536 bytes
	uint16_t	Bytes;       
	uint16_t 					: 15;
	uint16_t	EndOfTable		: 1;
} __attribute__((packed)) PRDTEntry;

typedef struct{
	//	The 
	uint64_t	Secondary				: 1;
	uint64_t	Primary					: 1;
	uint64_t	Slave					: 1;
	uint64_t							: 61;
}__packed ATAPIQueue;

typedef struct{
	CommonMutex							Mutex;
	struct ATAPIInterruptStackHeader	*Next;
	ATAPIQueue							Queue;
}__packed ATAPIInterruptStackHeader;