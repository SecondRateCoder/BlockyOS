#include "PCIe.h"
#include "kernel/libcrt/hardware/ACPI/ACPI.h"
#include "kernel/libcrt/hardware/IO/IO.h"

void *GetPCIeConfigurationBase(void *acpibase, uint32_t n){
	if(acpibase){
		void *Table = SearchACPITable("MCFG", acpibase);
		if(*((uint32_t *)(Table + __offsetof(SDTHeader_t, Revision))) >= XSDPRevision){
			XSDT_MCFG_t *_MCFG = (XSDT_MCFG_t *)Table;
			if(n > ((_MCFG->Header.Length - sizeof(SDTHeader_t)) / sizeof(XSDT_MCFGConfigurationEntry_t))){return false;}
			XSDT_MCFGConfigurationEntry_t *E = _MCFG->Table + n;
			void *Virtual = NULL;
			if(!(Virtual = MapVirtual((void *)E->ConfigurationBaseAddress))){
				//	Allocate the Pages required.
				Virtual = AllocateAlignedPages((void *)E->ConfigurationBaseAddress, nMB(E->EndingBus - E->StartingBus + 1), 
					(ReadWritable | SupervisorMode | PageLevelWriteThroughEnable), 0x0, 0x8);
			}
			return Virtual;
		}else{
			RSDT_MCFG_t *_MCFG = (RSDT_MCFG_t *)Table;
			if(n > ((_MCFG->Header.Length - sizeof(SDTHeader_t)) / sizeof(RSDT_MCFGConfigurationEntry_t))){return false;}
			RSDT_MCFGConfigurationEntry_t *E = _MCFG->Table + n;
			void *Virtual = NULL;
			if(!(Virtual = MapVirtual((void *)E->ConfigurationBaseAddress))){
				//	Allocate the Pages required.
				Virtual = AllocateAlignedPages((void *)E->ConfigurationBaseAddress, nMB(E->EndingBus - E->StartingBus + 1), 
					(ReadWritable | SupervisorMode | PageLevelWriteThroughEnable), 0x0, 0x8);
			}
			return Virtual;
		}
	}
}
bool ReadPCIeU32(void *acpibase, uint32_t *dataout, uint32_t n, uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset){
	void *Temp = GetPCIeConfigurationBase(acpibase, n);
	if(!Temp){(*dataout) = 0;			return false;}
	(*dataout) = *(uint32_t *)PCIeMakeAddress(Temp, bus, slot, func, offset);
	return true;
}
bool WritePCIeU32(void *acpibase, uint32_t datain, int32_t n, uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset){
	void *Temp = GetPCIeConfigurationBase(acpibase, n);
	if(!Temp){return false;}else{*(uint32_t *)PCIeMakeAddress(Temp, bus, slot, func, offset) = datain;}
	
	return true;
}

uint32_t PCIeBARSize(PCIDevice *dev, uint8_t BAR){
    uint32_t *bar_ptr = (uint32_t *)(((PCIHeader0x0 *)dev)->BAR + BAR);

    uint32_t orig_bar = *bar_ptr;
	bool IOEnable = dev->Command.IOSpace, MSEnable = dev->Command.MemorySpace;
	dev->Command.MemorySpace = false;
	dev->Command.IOSpace = false;

    //	Probe size
    *bar_ptr = 0xFFFFFFFF;
    uint32_t probed = *bar_ptr;

    //	Restore original BAR and Command state immediately
    *bar_ptr = orig_bar;
	if(IOEnable){dev->Command.IOSpace = true;}
	if(MSEnable){dev->Command.MemorySpace = true;}

    //	Mask flag bits and compute size
    if(orig_bar & 0x01){probed &= ~0x03;	//	I/O Space
	//	Memory Space
    }else{probed &= ~0x0F;}
    

    return (~probed) + 1;
}

void *PCIeResolveBar(PCIDevice *Device, uint8_t BAR){
	void *out = NULL;
	if(Device->HeaderType.Type == PCIGeneralDevice){
		uint64_t Physical = PCIGetBar((PCIHeader0x0 *)Device, BAR);
		if(Physical && !(out = MapVirtual((void *)Physical))){
			out = AllocateAlignedPages(((PCIHeader0x0 *)Device)->BAR[BAR].MSBAR.IsIOSpace? NULL: (void *)(((PCIHeader0x0 *)Device)->BAR[BAR].MSBAR._16ByteAlignedAddress), PCIeBARSize(Device, BAR), 
				//	We can simply align to 16 to appeal to both schemes.
				(ReadWritable | SupervisorMode), 0x0, 0x20);
		}
	}
	return ((PCIHeader0x0 *)Device)->BAR[BAR].IOBAR.IsIOSpace? (void *)((PCIHeader0x0 *)Device)->BAR[BAR].IOBAR._4ByteAlignedAddress: 0x00;
}

bool ReadPCIeVOIDPTR(void *acpibase, void *dataout, uint32_t datasize, uint32_t n, uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset){
	uint16_t _temp = offset;
	if((datasize % sizeof(uint32_t)) != 0){return false;}
	do{
		uint32_t temp = 0;
		if(ReadPCIeU32(acpibase, &temp, n, bus, slot, func, offset)){((uint32_t *)dataout)[(offset - _temp) / sizeof(uint32_t)] = temp;}else{return false;}
		offset += sizeof(uint32_t);
	}while(datasize -= sizeof(uint32_t));
	return true;
}
bool WritePCIeVOIDPTR(void *acpibase, void *datain, uint32_t datasize, uint32_t n, uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset){
	uint16_t _temp = offset;
	if((datasize % sizeof(uint32_t)) != 0){return false;}
	do{
		if(!WritePCIeU32(acpibase, *((uint32_t *)datain + ((offset - _temp) / sizeof(uint32_t))), n, bus, slot, func, offset)){return false;}
		offset += sizeof(uint32_t);
	}while(datasize -= sizeof(uint32_t));
	return true;
}

PCIDevice *PCIeSearchDevice(void *acpibase, PCIDeviceSpecifier Code, uint32_t N, uint32_t *bus, uint32_t *slot){
	uint32_t devicen = 0;
	static uint32_t _bus = 0, _slot = 0;
	if(!slot){slot = &_slot;}
	if(!bus){bus = &_bus;}
	*slot = 0;
	*bus = 0;
	void *base = NULL;
	do{
		base = GetPCIeConfigurationBase(acpibase, devicen);
		for(; (*bus) < PCIMaxBuses; ++(*bus)){
			for(; (*slot) < PCIMaxDevices; ++(*slot)){
				if(N){N--;}else{
					PCIDevice *dev = (PCIDevice *)PCIeMakeAddress(base, *bus, *slot, 0x0, 0x0);
					if(!memcmp(&(dev->DeviceCode), &Code, sizeof(PCIDeviceSpecifier))){return dev;}
				}
			}
		}
		devicen++;
	}while(base);
	return NULL;
}
bool PCIeDisableMsiX(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint32_t instance, uint32_t entry){
	uint32_t func = 0;
	UndefinedPCIeCapability *_MSIx = SearchPCIeCapabilitiesN(acpibase, n, bus, slot, &func, PCIeMSIxCapability, instance);
	if(_MSIx){
		void *Adr = GetPCIeConfigurationBase(acpibase, n);
		PCIeMSIxCapability_t *MSIx = (PCIeMSIxCapability_t *)_MSIx;
		PCIDevice *dev = (PCIDevice *)PCIeMakeAddress(Adr, bus, slot, func, 0x0);
		if(dev->HeaderType.Type == PCIGeneralDevice){
			uint8_t *bitmap = PCIeResolveBar(dev, MSIx->PendingBitArray.PBABARIndex) + MSIx->PendingBitArray.PBAByteOffset;
			bitmap[entry / 8] &= ~(1 << (entry % 8));
			return true;
		}
	}
	return false;
}
bool PCIeEnableMsiX(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint32_t instance, uint32_t entry){
	uint32_t func = 0;
	UndefinedPCIeCapability *_MSIx = SearchPCIeCapabilitiesN(acpibase, n, bus, slot, &func, PCIeMSIxCapability, instance);
	if(_MSIx){
		void *Adr = GetPCIeConfigurationBase(acpibase, n);
		PCIeMSIxCapability_t *MSIx = (PCIeMSIxCapability_t *)_MSIx;
		PCIDevice *dev = (PCIDevice *)PCIeMakeAddress(Adr, bus, slot, func, 0x0);
		if(dev->HeaderType.Type == PCIGeneralDevice){
			uint8_t *bitmap = PCIeResolveBar(dev, MSIx->PendingBitArray.PBABARIndex) + MSIx->PendingBitArray.PBAByteOffset;
			bitmap[entry / 8] |= (1 << (entry % 8));
			return true;
		}
	}
	return false;
}
MSIxTableEntry_t *PCIeGetMSIxEntry(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint32_t instance, uint32_t entry){
	uint32_t func = 0;
	UndefinedPCIeCapability *_MSIx = SearchPCIeCapabilitiesN(acpibase, n, bus, slot, &func, PCIeMSIxCapability, instance);
	if(_MSIx){
		void *Adr = GetPCIeConfigurationBase(acpibase, n);
		PCIeMSIxCapability_t *MSIx = (PCIeMSIxCapability_t *)_MSIx;
		PCIDevice *dev = (PCIDevice *)PCIeMakeAddress(Adr, bus, slot, func, 0x0);
		if(dev->HeaderType.Type == PCIGeneralDevice){
			return (MSIxTableEntry_t *)(PCIeResolveBar(dev, MSIx->PendingBitArray.PBABARIndex) + MSIx->PendingBitArray.PBAByteOffset) + entry;
		}
	}
	return NULL;
}

UndefinedPCIeCapability *SearchPCIeCapabilitiesN(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint32_t *func, PCIeCapabilitiesDeviceType type, uint32_t encounter){
	for(uint32_t cc = 0; cc < PCIMaxPhysicalFunctions; ++cc){
		UndefinedPCIeCapability *out = ReadPCIeCapabilities(acpibase, n, bus, slot, cc);
		if((out->ExtHeader.ExtCAPID == type) && !encounter){
			if(func){*func = cc;}
			return out;
		}else{encounter--;}
		mfree(out);
	}
	return NULL;
}

UndefinedPCIeCapability *ReadPCIeCapabilities(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint16_t func){
	void *Adr = GetPCIeConfigurationBase(acpibase, n);
	PCIDevice *dev = (PCIDevice *)PCIeMakeAddress(Adr, bus, slot, func, 0x0);
	PCIeCapabilitiesHeader *Hdr = NULL;
	switch(dev->HeaderType.Type){
		case PCIGeneralDevice:		{Hdr = (PCIeCapabilitiesHeader *)(PCIeMakeAddress(Adr, bus, slot, func, ((PCIHeader0x0 *)dev)->CapabilitiesListOffset));	break;}
		case PCItoPCIBridge:	{Hdr = (PCIeCapabilitiesHeader *)(PCIeMakeAddress(Adr, bus, slot, func, ((PCIHeader0x1 *)dev)->CapabilitiesListOffset));	break;}
		case PCItoCardBusBridge:{Hdr = (PCIeCapabilitiesHeader *)(PCIeMakeAddress(Adr, bus, slot, func, ((PCIHeader0x2 *)dev)->CapabilitiesListOffset));	break;}
		default: {return NULL;}
	}
	UndefinedPCIeCapability *Temp = NULL;
	switch(Hdr->ExtCAPID){
		case PCIeMSIxCapability: {
			Temp = (UndefinedPCIeCapability *)mmalloczero(sizeof(PCIeMSIxCapability_t));
			memcpy(Temp, Hdr, sizeof(PCIeMSIxCapability_t));
			break;
		} case PCIeGenericCapabilites: {
			if(Hdr->Next >= 0x100){
				Temp = (UndefinedPCIeCapability *)mmalloczero(sizeof(PCIeSRIOVExtendedCapabilities_t));
				memcpy(Temp, Hdr, sizeof(PCIeSRIOVExtendedCapabilities_t));
			}else{
				Temp = (UndefinedPCIeCapability *)mmalloczero(Hdr->Version >= 2? 
					sizeof(PCIeCapablitiesVersion2): sizeof(PCIeCapablitiesVersion1));
				memcpy(Temp, Hdr, Hdr->Version >= 2? sizeof(PCIeCapablitiesVersion2): 
					sizeof(PCIeCapablitiesVersion1));
			}
			break;
		} case PCIeSecondaryExtendedCapabilities: {
			Temp = (UndefinedPCIeCapability *)mmalloczero(sizeof(PCIeSecondaryExtendedCapabilities_t));
			memcpy(Temp, Hdr, sizeof(PCIeSecondaryExtendedCapabilities_t));
			break;
		} case PCIeAdvancedErrorReporting: {
			Temp = (UndefinedPCIeCapability *)mmalloczero(sizeof(PCIeAERExtendedCapability_t));
			memcpy(Temp, Hdr, sizeof(PCIeAERExtendedCapability_t));
			break;
		} 
	}
	return Temp;
}
