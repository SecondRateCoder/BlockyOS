#pragma once
#include "_ATAPI.h"

void ATAPIGetPorts(PCIDevice *device, uint32_t *nativemode, uint32_t *pcommand, uint32_t *pcontrol, uint32_t *scommand, uint32_t *scontrol, uint32_t *bmba, uint32_t *pIVector, uint32_t *sIVector){
	if(device){
		if(device->HeaderType.Type != PCIGeneralDevice){return;}
		ATAPI_PCIProgIF pf = *((ATAPI_PCIProgIF *)(&device->DeviceCode.ProgIF));
		if(pf.PrimaryChannelMode){
			if(pcommand){*pcommand = *((uint32_t *)&((PCIHeader0x0 *)device)->BAR[0]);}
			if(pcontrol){*pcontrol = *((uint32_t *)&((PCIHeader0x0 *)device)->BAR[1]);}
			if(scommand){*scommand = *((uint32_t *)&((PCIHeader0x0 *)device)->BAR[2]);}
			if(scontrol){*scontrol = *((uint32_t *)&((PCIHeader0x0 *)device)->BAR[3]);}
			if(bmba){*bmba = *((uint32_t *)&((PCIHeader0x0 *)device)->BAR[4]);}
			if(nativemode){*nativemode = true;}
		}
	}
	if(pcommand){*pcommand = ATAPIPrimaryCommandPort;}
	if(pcontrol){*pcontrol = ATAPIPrimaryControlPort;}
	if(scommand){*scommand = ATAPISecondaryCommandPort;}
	if(scontrol){*scontrol = ATAPISecondaryControlPort;}
	if(pIVector){*pIVector = 0x20 + 0x14;}
	if(sIVector){*sIVector = 0x20 + 0x15;}
	if(nativemode){*nativemode = false;}
	if(bmba){*bmba = UINT32_MIN;}
}
void ATAPIPort(void *acpibase, uint32_t APIC, PCIDevice *device, uint32_t *nativemode, uint32_t *pcommand, uint32_t *pcontrol, 
	uint32_t *scommand, uint32_t *scontrol, uint32_t *bmba, uint32_t *pIVector, uint32_t *sIVector
){
	static uint32_t _nativemode, _pcommand, _pcontrol, _scommand, _scontrol, _bmba, _pIVector, _sIVector;
	if(device){
		ATAPIGetPorts(device, &_nativemode, &_pcommand, &_pcontrol, &_scommand, &_scontrol, &_bmba, NULL, NULL);
		void *_MADT = SearchACPITable("MADT", acpibase);
		if(((SDTHeader_t *)_MADT)->Revision >= XSDPRevision){
			XSDT_MADT_t *MADT = _MADT;
			uint64_t Offset = 0x00;
			for(uint32_t cc = 0; cc < ((MADT->Header.Length - sizeof(XSDT_MADT_t)) / sizeof(XSDT_MADTEntry)); ++cc){
				XSDT_MADTEntry *E = (void *)MADT->Table + Offset;
				if(MADT->Table[cc].Type == XSDT_IOAPICInterruptSourceOverride){
					XSDT_IOAPICInterruptSourceOverride_t *Override = (XSDT_IOAPICInterruptSourceOverride_t *)E;
					if((Override->IRQSource == 0x14 || Override->IRQSource == 0x15) && (pIVector || sIVector)){
						IOAPICInterruptRegister IR = ReadIoApicInterruptRegister(
							acpibase, APIC, Override->IRQSource);
						IR.Register.Vector = Override->IRQSource == 0x14 && pIVector? *pIVector: 
							(Override->IRQSource == 0x15 && sIVector? *sIVector: 0x00);
						if(Override->IRQSource == 0x14){_pIVector = *pIVector;}else 
						if(Override->IRQSource == 0x15){_sIVector = *sIVector;}
						WriteIoApicInterruptRegister(acpibase, APIC, Override->IRQSource, IR);
						continue;
					}
					if(_pIVector && _sIVector){break;}
				}
				Offset += E->RecordTypeLength;
			}
		}else{
			RSDT_MADT_t *MADT = _MADT;
			uint64_t Offset = 0x00;
			for(uint32_t cc = 0; cc < ((MADT->Header.Length - sizeof(RSDT_MADT_t)) / sizeof(RSDT_MADTEntry)); ++cc){
				RSDT_MADTEntry *E = (void *)MADT->Table + Offset;
				if(MADT->Table[cc].Type == RSDT_IOAPICInterruptSourceOverride){
					RSDT_IOAPICInterruptSourceOverride_t *Override = (RSDT_IOAPICInterruptSourceOverride_t *)E;
					if((Override->IRQSource == 0x14 || Override->IRQSource == 0x15) && (pIVector || sIVector)){
						IOAPICInterruptRegister IR = ReadIoApicInterruptRegister(
							acpibase, APIC, Override->IRQSource);
						IR.Register.Vector = Override->IRQSource == 0x14 && pIVector? *pIVector: 
							(Override->IRQSource == 0x15 && sIVector? *sIVector: 0x00);
						if(Override->IRQSource == 0x14){_pIVector = *pIVector;}else 
						if(Override->IRQSource == 0x15){_sIVector = *sIVector;}
						WriteIoApicInterruptRegister(acpibase, APIC, Override->IRQSource, IR);
						continue;
					}
					if(_pIVector && _sIVector){break;}
				}
				Offset += E->RecordTypeLength;
			}
		}
		// _pIVector = *pIVector;			_sIVector = *sIVector;
	}
	if(nativemode){*nativemode = _nativemode;}
	if(pcommand){*pcommand = _pcommand;}
	if(pcontrol){*pcontrol = _pcontrol;}
	if(scommand){*scommand = _scommand;}
	if(scontrol){*scontrol = _scontrol;}
	if(pIVector){*pIVector = _pIVector;}
	if(sIVector){*sIVector = _sIVector;}
	if(bmba){*bmba = _bmba;}
}

uint8_t ATATPITransferOngoing(uint16_t bmba, uint16_t pcmd){
	//	Read Bus Master Status Register (offset 0x02)
	uint8_t bm_status = __inb(bmba + 0x02);

	//	Check for Bus Master errors
	if(bm_status & 0x02){
		//	Clear error bit by writing 1 back to it, and stop bus master
		__outb(bmba + 0x02, bm_status | 0x02);
		__outb(bmba, 0x00); //	Stop DMA
		return 2; //	Error state
	}

	//	Check if the transfer interrupt bit is set (Bit 2) or active bit (Bit 0) is cleared
	//	If Bit 2 is 1, the transfer has finished.
	if(bm_status & 0x04){
		//	Acknowledge the interrupt by writing 1 back to bit 2
		__outb(bmba + 0x02, bm_status | 0x04);
		//	Stop the Bus Master transfer
		__outb(bmba, 0x00);

		//	Double-check the primary command port status to ensure DRQ is cleared and Busy (BSY) is 0
		uint8_t status = __inb(pcmd);
		if(!(status & 0x80) && !(status & 0x08)){return 0;}
	}
	return 1; // Still busy / transfer ongoing
}

ISRCallbackDefinition(ATAPI){
	static uint32_t ntvmode, pcmd, pctrl, scmd, sctrl, bmba;
	ATAPIPort(NULL, 0x00, NULL, &ntvmode, &pcmd, &pctrl, &scmd, &sctrl, &bmba, NULL, NULL);
	ATAPIInterruptStackHeader *IST = (ATAPIInterruptStackHeader *)Frame->StackHeader, *Previous = NULL;
	while(IST->Next){Previous = IST;	IST = (ATAPIInterruptStackHeader *)IST->Next;}
	while(ATATPITransferOngoing(bmba, pcmd)){;}
	mfree(Previous->Next);		Previous->Next = NULL;
	UnlockMutex(IST->Mutex);
	ISRCallbackReturn;
}
void UpdateATAPIInterruptStackHeader(CommonMutex Mtx, ATAPIQueue q){
	uint32_t pVector, sVector;
	ATAPIPort(NULL, 0x00, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &pVector, &sVector);
	if(q.Primary){
		ATAPIInterruptStackHeader *ATAPI = (ATAPIInterruptStackHeader *)ISRGetStackHeader(pVector);
		while(ATAPI->Next){ATAPI = (ATAPIInterruptStackHeader *)ATAPI->Next;}
		ATAPI->Next = mcalloc(1, sizeof(ATAPIInterruptStackHeader));
		*((ATAPIInterruptStackHeader *)ATAPI->Next) = (ATAPIInterruptStackHeader){
			.Mutex = Mtx, .Next = NULL, .Queue = q};
	}
	if(q.Secondary){
		ATAPIInterruptStackHeader *ATAPI = ISRGetStackHeader(sVector);
		while(ATAPI->Next){ATAPI = (ATAPIInterruptStackHeader *)ATAPI->Next;}
		ATAPI->Next = mcalloc(1, sizeof(ATAPIInterruptStackHeader));
		*((ATAPIInterruptStackHeader *)ATAPI->Next) = (ATAPIInterruptStackHeader){
			.Mutex = Mtx, .Next = NULL, .Queue = q};
	}
}

ATAPIQueue ConfigureATAPIController(void *acpibase, uint32_t IOAPIC, PCIDevice *device, uint8_t Priviledge, uint8_t Channel, uint8_t Drive, ...){
	va_list ls;	va_start(ls, Drive);
	uint32_t pv = ((Channel == 0) || (Channel == 3))? va_arg(ls, uint32_t): 0x00, 
			sv = (Channel == 1) || (Channel == 3)? va_arg(ls, uint32_t): 0x00;
	ATAPIPort(acpibase, IOAPIC, device, NULL, NULL, NULL, NULL, NULL, NULL, &pv, &sv);
	return (ATAPIQueue){
		.Primary = (Channel == 0) || (Channel == 3), 
		.Secondary = (Channel == 1) || (Channel == 3), 
		.Slave = (Drive == 1)
	};
}

uint32_t _ATAPISectorSize(bool slave, uint32_t cmd, uint32_t ctrl){
	//	Wait for controller status ready
	//	Build the 12-byte SCSI READ CAPACITY (10) command packet (Opcode 0x25)
	//	Send the command packet via word-out
	//	Wait for the device to process and return data (DRQ set)
	//	Read the 8-byte response (4 words)
	//		Bytes 0-3: Last Logical Block Address
	//		Bytes 4-7: Block Length (Sector Size) in Big-Endian format
	while(1){
		uint8_t status = __inb(cmd);
		//	Error
		if((status & 0x01) == 1){return 0;}
		//	DRQ clear, Busy clear / Ready
		if(!(status & 0x80) && (status & 0x08)){break;}
	}
	__outb(cmd + ATAPI_DRIVE_SELECT, ATAPI_REPORT_LUNS | (slave << 4));

	ATAPICommand rcmd = {0};
	// read_capacity_cmd[0] = 0x25; // READ CAPACITY (10)
	rcmd.Command = ATAPI_READ_CAPACITY;
	__outsw(cmd + ATAPI_DATA, (uint16_t *)&rcmd, sizeof(ATAPICommand));
	
	while(true){
		uint8_t status = __inb(ctrl);
		if(status & 0x01){ return 0; } // Error
		if(!(status & 0x80) && (status & 0x08)){ break; }
	}
	
	uint32_t response[2] = {0};
	uint16_t *buf_ptr = (uint16_t *)response;
	for(int i = 0; i < 4; i++){buf_ptr[i] = __inw(cmd + ATAPI_DATA);}
	
	//	Extract bytes 4-7 and convert from Big-Endian to CPU Endianness
	uint8_t *raw_bytes = (uint8_t *)response;
	uint32_t sector_size = ((uint32_t)raw_bytes[4] << 24) |
						  ((uint32_t)raw_bytes[5] << 16) |
						  ((uint32_t)raw_bytes[6] << 8)  |
						  (uint32_t)raw_bytes[7];
	//	Returns 2048 for standard DVDs/CDs, 512 for specific media
	return sector_size;
}

//	32: Primary Drive Sector Size
//	32: Secondary Drive Sector Size
uint64_t ATAPISectorSize(ATAPIQueue queue){
	uint64_t out = 0x00;
	uint32_t ntvmode, pcmd, pctrl, scmd, sctrl, bmba;
	ATAPIPort(NULL, 0x00, NULL, &ntvmode, &pcmd, &pctrl, &scmd, &sctrl, &bmba, NULL, NULL);
	return ((queue.Primary? _ATAPISectorSize(queue.Slave, pcmd, pctrl): 0x00) << 32) + 
		(queue.Secondary? _ATAPISectorSize(queue.Slave, scmd, sctrl): 0x00);
}

ATAPIQueue ATAPIGetQueue(bool slave, uint32_t Channel){
	return (ATAPIQueue){
		.Primary = (Channel == 0) || (Channel == 3), 
		.Secondary = (Channel == 1) || (Channel == 3), 
		.Slave = slave
	};
}

uint32_t _ATAPIReadRaw(bool slave, uint32_t cmd, uint32_t ctrl, uint32_t start, uint32_t bytes, void *buffer){
	{	//	Safely handle Data.
		ml_t temp = descinfo(buffer);
		if(temp.free && temp.mdesc.nbytes < bytes){return bytes;}
	}
	uint32_t SectorSize = _ATAPISectorSize(slave, cmd, ctrl), ntvmode, bmba = 0x00;
	uint32_t lba = start / SectorSize;
	ATAPIPort(NULL, 0x00, NULL, &ntvmode, NULL, NULL, NULL, NULL, &bmba, NULL, NULL);
	uint32_t sectors = (bytes / SectorSize) +  ((bytes % SectorSize) != 0);
	// The command
	ATAPICommand read_cmd = {
		.Command = ATAPI_READ12, 
		.Payload = {
			0x0, (lba >> 0x18) & 0xFF, (lba >> 0x10) & 0xFF, (lba >> 0x08) & 0xFF, (lba >> 0x00) & 0xFF, 
			(sectors >> 0x18) & 0xFF, (sectors >> 0x10) & 0xFF, (sectors >> 0x08) & 0xFF, (sectors >> 0x00) & 0xFF, 
			0x0, 0x0
		}
	};
	__outb(cmd + ATAPI_DRIVE_SELECT, ATAPI_REPORT_LUNS | (slave << 4)); // Drive select
	SleepMS(10);
	__outb(cmd + ATAPI_ERROR_R, 0x00); 
	__outb(cmd + ATAPI_LBA_MID, 2048 & 0xFF);
	__outb(cmd + ATAPI_LBA_HIGH, 2048 >> 8);
	__outb(cmd + ATAPI_COMMAND_REGISTER, ATAPI_REPORT_LUNS); // Packet command
	SleepMS(10); // I think we might need this delay, not sure, so keep this
 
	//	Wait for status
	while(1){
		uint8_t status = __inb(cmd);
		if((status & 0x01) == 1){return 1;}
		if(!(status & 0x80) && (status & 0x08)){break;}
		SleepMS(10);
	}

	if(ntvmode){
		uint8_t *bf = AllocateAlignedPages(NULL, sizeof(PRDTEntry) * __roundup(bytes, LegacySectorSize), (ReadWritable | SupervisorMode), 0x00, 0x20);
		__outl(bmba + 0x04, (uint32_t)MapPhysical(bf));
		uint64_t tempbf = (uint64_t)MapPhysical(buffer);
		while(bytes){
			*((PRDTEntry *)bf) = (PRDTEntry){.Bytes = bytes >= 65536? 0: bytes, .EndOfTable = bytes? 1: 0, .PhysicalAddress = tempbf};
			tempbf += (bytes < 65536? 0: bytes - 65536);
			bytes = bytes < 65536? 0: bytes - 65536;
			bf += sizeof(PRDTEntry);
		}
		__outb(bmba, 0x09);
	}
	//	Send command
	__outsw(cmd + ATAPI_DATA, (uint16_t *)(&read_cmd), 12);
	if(!ntvmode){
		// Read words
		for(uint32_t i = 0; i < sectors; i++){
			while(true){
				uint8_t status = __inb(ctrl);
				if(status & 0x01){return 1;}
				if(!(status & 0x80) && (status & 0x08)){break;}
			}
			__insw(cmd + ATAPI_DATA, (uint16_t *)((uint8_t *)(MapPhysical(buffer) + (i * 0x800))), 
				__inb(cmd + ATAPI_LBA_HIGH) << 8 | __inb(cmd + ATAPI_LBA_MID) / 2); // Read it
		}
	}
	return 0;
}
uint32_t ATAPIReadRaw(ATAPIQueue q, CommonMutex Mtx, uint8_t select, uint32_t start, uint32_t bytes, void *buffer){
	uint32_t pcmd, pctrl, scmd, sctrl;
	UpdateATAPIInterruptStackHeader(Mtx, q);
	ATAPIPort(NULL, 0x00, NULL, NULL, &pcmd, &pctrl, &scmd, &sctrl, NULL, NULL, NULL);
	if(select == 0 && q.Primary){return _ATAPIReadRaw(q.Slave, pcmd, pctrl, start, bytes, buffer);}else 
	if(select == 1 && q.Secondary){return _ATAPIReadRaw(q.Slave, scmd, sctrl, start, bytes, buffer);}
	return bytes;
}

uint32_t _ATAPIWriteRaw(bool slave, uint32_t cmd, uint32_t ctrl, uint32_t start, uint32_t bytes, void *buffer){
    {   // Safely handle Data.
        ml_t temp = descinfo(buffer);
        if(temp.free && temp.mdesc.nbytes < bytes){return bytes;}
    }
    uint32_t SectorSize = _ATAPISectorSize(slave, cmd, ctrl), ntvmode, bmba, 
            lba = start / SectorSize, sectors = (bytes / SectorSize);
    ATAPIPort(NULL, 0x00, NULL, &ntvmode, NULL, NULL, NULL, NULL, &bmba, NULL, NULL);
    
    // The write command (using SCSI WRITE (10) opcode 0x2A)
    ATAPICommand write_cmd = {
        .Command = ATAPI_WRITE12 , 
        .Payload = {
            0x0, 
            (lba >> 0x18) & 0xFF, (lba >> 0x10) & 0xFF, (lba >> 0x08) & 0xFF, (lba >> 0x00) & 0xFF, 
            (sectors >> 0x18) & 0xFF, (sectors >> 0x10) & 0xFF, (sectors >> 0x08) & 0xFF, (sectors >> 0x00) & 0xFF, 
            0x0, 0x0
        }
    };
    
    __outb(cmd + ATAPI_DRIVE_SELECT, ATAPI_REPORT_LUNS | (slave << 4)); // Drive select
    SleepMS(10);
    __outb(cmd + ATAPI_ERROR_R, 0x00); 
    __outb(cmd + ATAPI_LBA_MID, 2048 & 0xFF);
    __outb(cmd + ATAPI_LBA_HIGH, 2048 >> 8);
    __outb(cmd + ATAPI_COMMAND_REGISTER, ATAPI_REPORT_LUNS); // Packet command
    SleepMS(10);
 
    // Wait for status
    while(1){
        uint8_t status = __inb(cmd);
        if((status & 0x01) == 1){return 1;}
        if(!(status & 0x80) && (status & 0x08)){break;}
        SleepMS(10);
    }

    if(ntvmode){
        uint8_t *bf = AllocateAlignedPages(NULL, sizeof(PRDTEntry) * __roundup(bytes, LegacySectorSize), (ReadWritable | SupervisorMode), 0x00, 0x20);
        __outl(bmba + 0x04, (uint32_t)MapPhysical(bf));
        uint64_t tempbf = (uint64_t)MapPhysical(buffer);
        uint32_t remaining_bytes = bytes;
        while(remaining_bytes){
            uint32_t chunk = remaining_bytes >= 65536 ? 0 : remaining_bytes;
            uint32_t step = remaining_bytes >= 65536 ? 65536 : remaining_bytes;
            
            *((PRDTEntry *)bf) = (PRDTEntry){
                .Bytes = chunk, 
                .EndOfTable = remaining_bytes <= 65536 ? 1 : 0, 
                .PhysicalAddress = tempbf
            };
            tempbf += step;
            remaining_bytes -= step;
            bf += sizeof(PRDTEntry);
        }
        // Set Bus Master command for WRITE (Bit 3 clear for Write direction, Bit 0 set to start)
        __outb(bmba, 0x01);
    }

    // Send command packet
    __outsw(cmd + ATAPI_DATA, (uint16_t *)(&write_cmd), 12);

    if(!ntvmode){
        // Write words via PIO
        for(uint32_t i = 0; i < sectors; i++){
            while(true){
                uint8_t status = __inb(ctrl);
                if(status & 0x01){return 1;}
                if(!(status & 0x80) && (status & 0x08)){break;}
            }
            uint32_t trans_size = __inb(cmd + ATAPI_LBA_HIGH) << 8 | __inb(cmd + ATAPI_LBA_MID);
            __outsw(cmd + ATAPI_DATA, (uint16_t *)((uint8_t *)MapPhysical(buffer) + (i * 0x800)), trans_size / 2);
        }
    }
    return 0;
}
uint32_t ATAPIWriteRaw(ATAPIQueue q, CommonMutex Mtx, uint8_t select, uint32_t start, uint32_t bytes, void *buffer){
	uint32_t pcmd, pctrl, scmd, sctrl;
	UpdateATAPIInterruptStackHeader(Mtx, q);
	ATAPIPort(NULL, 0x00, NULL, NULL, &pcmd, &pctrl, &scmd, &sctrl, NULL, NULL, NULL);
	if(select == 0 && q.Primary){return _ATAPIWriteRaw(q.Slave, pcmd, pctrl, start, bytes, buffer);}else 
	if(select == 1 && q.Secondary){return _ATAPIWriteRaw(q.Slave, scmd, sctrl, start, bytes, buffer);}
	return bytes;
}