#include "PCI.h"
// Code adapted from (https://osdev.wiki/wiki/PCI)

uint16_t pciReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
	uint32_t address = (
		((uint32_t)bus << 16) | ((uint32_t)slot << 16) | 
		((uint32_t)func << 16) | ((uint32_t)offset << 16) | 
		((uint32_t)0x80000000)
	);
	outl(PCICONFIGADDR, address);
	uint16_t out = (uint16_t)((inl(PCICONFIGDATA) >> ((offset & 2) * 8)) & 0xFFFF);
	return out;
}

inline bool pciCheckDevice(uint8_t bus, uint8_t slot){
	return (pciReadWord(bus, slot, 0, 0) != 0);
}

inline bool pciCheckFunc(uint8_t bus, uint8_t slot, uint8_t func){
	return (pciReadWord(bus, slot, func, 0) != 0);
}

bool getPCICommon(uint8_t bus, uint8_t slot, PCICommon *out){
	if(pciCheckDevice(bus, slot)){
		*out = (PCICommon){
			.DeviceID = PCIDeviceID(bus, slot),
			.VendorID = PCIVendorID(bus, slot),
			.StatusCode = PCIStatusCode(bus, slot),
			.CommandCode = PCICommandCode(bus, slot),
			.ClassCode = PCIClassCode(bus, slot),
			.SubClassCode = PCISubClassCode(bus, slot),
			.ProgIF = PCIProgIF(bus, slot),
			.RevisionID = PCIRevisionID(bus, slot),
			.BIST = PCIBIST(bus, slot),
			.HeaderType = PCIHeaderType(bus, slot),
			.LatencyTimer = PCILatencyTimer(bus, slot),
			.CacheLineSize = PCICacheLineSize(bus, slot)
		};
		return true;
	}
	return false;
}

void PCIGetFullHeader(void *out, uint8_t bus, uint8_t slot, uint8_t func){
    if(getPCICommon(bus, slot, out)){
        switch(((PCICommon *)out)->HeaderType){
            case 0: {
                for(uint8_t cc = 0; cc < ((sizeof(PCIHeader0x0) - sizeof(PCICommon)))/sizeof(uint16_t); ++cc){
                    ((uint16_t *)out)[sizeof(PCICommon) + cc] = pciReadWord(bus, slot, func, cc * sizeof(uint16_t));
                }
            }
            case 1: {
                for(uint8_t cc = 0; cc < ((sizeof(PCIHeader0x1) - sizeof(PCICommon)))/sizeof(uint16_t); ++cc){
                    ((uint16_t *)out)[sizeof(PCICommon) + cc] = pciReadWord(bus, slot, func, cc * sizeof(uint16_t));
                }
            }
            case 2: {
                for(uint8_t cc = 0; cc < ((sizeof(PCIHeader0x2) - sizeof(PCICommon)))/sizeof(uint16_t); ++cc){
                    ((uint16_t *)out)[sizeof(PCICommon) + cc] = pciReadWord(bus, slot, func, cc * sizeof(uint16_t));
                }
            }
        }
    }
}