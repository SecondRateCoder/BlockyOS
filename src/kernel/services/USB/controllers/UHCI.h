#pragma once

//*	https://wiki.osdev.org/Universal_Host_Controller_Interface
// #include "kernel/libcrt/hardware/PCIe/Devices.h"
#include "kernel/services/USB/service.h"
// #include "kernel/libcrt/hardware/PCIe/PCIe.h"
// #include "kernel/libcrt/hardware/PCIe/PCI.h"
#include "kernel/libcrt/hardware/IO/IO.h"

#define UHCIPortNStatusControl(N)	(UHCIPortStatusControl + (sizeof(UHCIPortStatusControl_t) * (N)))
enumdef(uint32_t, UHCIRegister){
	UHCICommandRegister = 0x00, 
	UHCIStatusRegister = 0x02, 
	UHCIInterruptEnable = 0x04, 
	//	Number of the currently processed entry of the Frame List. 
	UHCIFrameNumber = 0x06, 
	//	32-bit physical address of Frame List. 
	//	Address has to be aligned to 4 Kb (first 12 bits are zero). 
	//	The Frame List must contain 1024 entries. 
	UHCIFrameListBaseAddress = 0x08, 
	//	This port sets timing of frame. 
	//	Should be 0x40. 
	UHCIStartOfFrameModify = 0x0C, 
	UHCIPort1StatusControl = 0x010, 
	UHCIPortStatusControl = UHCIPort1StatusControl
};
typedef struct{
	uint16_t		Run				: 1;
	uint16_t		HostCtrlReset	: 1;
	uint16_t		GlobalReset		: 1;
	uint16_t		GlobalSuspend	: 1;
	uint16_t		GlobalResume	: 1;
	//	Clears Run bit after each transaction.
	uint16_t		SoftwareDebug	: 1;
	uint16_t		ConfigureSuccess: 1;
	//	0 = Max Size is 32 bytes
	//	1 = Max Size is 64 bytes
	uint16_t		MaxPacketSize	: 1;
	uint16_t						: 8;
}__packed UHCICommandRegister_t;
typedef struct{
	uint16_t		InterruptPending				: 1;
	uint16_t		USBError						: 1;
	uint16_t		ResumeDetected					: 1;
	uint16_t		HostSystemError					: 1;
	uint16_t		HostControllerProcessingError	: 1;
	uint16_t		HostControllerHalted			: 1;
	uint16_t										: 10;
}__packed UHCIStatusRegister_t;
typedef struct{
	uint16_t		ErrorInterruptEnable			: 1;
	uint16_t		ResumeInterruptEnable			: 1;
	uint16_t		InterruptOnTransferComplete		: 1;
	uint16_t		ShortPacketInterruptEnable		: 1;
	uint16_t										: 12;
}__packed UHCIInterruptEnableRegister_t;
typedef struct{
	uint16_t				ConnectStatus			: 1;
	uint16_t Write1ToClear	ConnectStatusChange		: 1;
	uint16_t				PortEnable				: 1;
	uint16_t Write1ToClear	PortEnableChange		: 1;
	const uint16_t			LineStatus				: 2;
	uint16_t				ResumeDetect			: 1;
	const uint16_t			PortExist				: 1;
	const uint16_t			LowSpeedDevice			: 1;
	uint16_t				PortReset			: 1;
	uint16_t										: 2;
	uint16_t				PortSuspended			: 1;
	uint16_t										: 3;
}__packed UHCIPortStatusControl_t;

enumdef(uint8_t, UHCIFrameListEntryType){TransferDescriptor = 0x00, QueueHead = 0x01};
typedef struct{
	uint32_t		EmptyFrame						: 1;
	uint32_t		MemoryStructureType				: 1;
	uint32_t										: 2;
	uint32_t		PhysicalPointer					: 28;
}__packed UHCIFrameListEntry_t;
//	For this Driver HorizontalPointer is for persistent Requests, whilst
//		VerticalPointer is for temporary Operations.
typedef struct{
	UHCIFrameListEntry_t	VerticalPointer, 
							HorizontalPointer;
}__packed UHCIQueueHead_t;

typedef UHCIFrameListEntryType UHCITransferDescriptorNextDescriptorType;
typedef struct{
	struct{
		uint32_t	NullEntry						: 1;
		uint32_t	MemoryStructureType				: 1;
		//	This tells the Controller to point to the Entry pointed to here.
		uint32_t	ToNext							: 1;
		uint32_t									: 1;
		uint32_t	PhysicalPointer					: 28;
	}__packed		NextDescriptor;
	struct{
		uint32_t	TransferLength					: 11;
		uint32_t									: 5;
		uint32_t	BitStuffError					: 1;
		uint32_t	TimeoutCRC						: 1;
		uint32_t	NonAcknowledged					: 1;
		uint32_t	BabbleDetected					: 1;
		uint32_t	DataBufferError					: 1;
		uint32_t	Stalled							: 1;
		uint32_t	Active							: 1;
		uint32_t	InterruptOnComplete				: 1;
		uint32_t	Isochronous						: 1;
		uint32_t	LowSpeed						: 1;
		uint32_t	ErrorCounter					: 2;
		//	 Continue execution from horizontal QH pointer.
		uint32_t	ShortPacketDetect				: 1;
		uint32_t									: 3;
	}__packed		Status;
	struct{
		uint32_t	PacketType						: 8;
		uint32_t	Device							: 6;
		uint32_t	Endpoint						: 4;	
		uint32_t	DataToggle						: 1;	
		uint32_t									: 1;	
		uint32_t	MaximumLength					: 11;
	}__packed		PacketHeader;
	uint32_t		PhysicalBufferAddress;
	struct{
		uint128_t	Payload;
	}__packed Undefined;
}__packed UHCITransferDescriptor_t;

CommonMutex ConfigureUHCIController(void *acpibase, uint32_t IOAPIC, uint32_t N, uint32_t FrameListSize, 
	uint8_t Privilege, ISRCallback *CB, void *CallbackStackHeader
){
	FrameListSize = __min(__roundup(FrameListSize, 1024), 1024);
	uint32_t bus, slot;
	PCIDevice *dev = PCIeSearchDevice(acpibase, GetPCIeStruct(SerialBus_USB_UHCI), N, &bus, &slot);
	if(dev->HeaderType.Type == PCIGeneralDevice){
		DisableLocalAPIC(acpibase);
		WritePCIeU32(acpibase, 0x2000, N, bus, slot, 0x00, 0xC0);
		PCIHeader0x0 *_0x0 = (PCIHeader0x0 *)dev;
		_0x0->This.Command.BusMaster = true;
		_0x0->This.Command.IOSpace = true;

		SleepMS(10);
		uint32_t R = UHCIReadRegister(acpibase, N, UHCICommandRegister);
		UHCICommandRegister_t CR = *((UHCICommandRegister_t *)&R);
		CR.HostCtrlReset = true;
		UHCIWriteRegister(acpibase, N, UHCICommandRegister, *((uint32_t *)&CR));
		while(true){
			R = UHCIReadRegister(acpibase, N, UHCICommandRegister);
			CR = *((UHCICommandRegister_t *)&R);
			if(!CR.HostCtrlReset){break;}
		}

		CR.GlobalReset = true;
		UHCIWriteRegister(acpibase, N, UHCICommandRegister, *((uint32_t *)&CR));
		SleepMS(50);
		CR.GlobalReset = false;
		UHCIWriteRegister(acpibase, N, UHCICommandRegister, *((uint32_t *)&CR));
		SleepMS(10);
		
		//	We allocate in the Lower 32-bits of Memory.
		void *FrameList = AllocateAlignedPagesFromRange(NULL, UINT32_MAX, FrameListSize, ReadWritable | SupervisorMode, 0x00, PAGE_SIZE)/*, 
				*PFrameList = MapPhysical(FrameList)*/;
		for(uint32_t i = 0; i < 1024; i++){((uint32_t *)FrameList)[i] = 1;}//	Set Terminate bit
		UHCIWriteRegister(acpibase, N, UHCIFrameListBaseAddress, (uint32_t)(uintptr_t)FrameList);

		R = UHCIReadRegister(acpibase, N, UHCIInterruptEnable);
		UHCIInterruptEnableRegister_t IE = *((UHCIInterruptEnableRegister_t *)&R);
		IE.ResumeInterruptEnable = true;
		UHCIWriteRegister(acpibase, N, UHCIInterruptEnable, *((uint32_t *)&IE));

		uint8_t IST;
		IDTEntrySegmentSelector64 sselector = {0};
		AllocateIST(&IST, &sselector);
		uint8_t IV;
		IOAPICInterruptRegister IR = ReadIoApicInterruptRegister(acpibase, IOAPIC, _0x0->InterruptLine);
		AllocateInterruptVector(&IV);
		ISRSetCallback(acpibase, IV, Privilege, IST, CallbackStackHeader, sselector, true, CB);
		IR.Register.Vector = IV;
		WriteIoApicInterruptRegister(acpibase, IOAPIC, _0x0->InterruptLine, IR);
		InitStaticMutex(Mtx);
		EnableLocalAPIC(acpibase);
		return Mtx;
	}
	return NULL;
}

uint32_t UHCIReadRegister(void *acpibase, uint32_t N, UHCIRegister r){
	uint32_t port = 0x00;
	if(GetUHCIControllerPort(acpibase, N, &port)){
		switch(r){
			case UHCIStatusRegister:
			case UHCIInterruptEnable:
			case UHCIFrameNumber:
			case UHCIPort1StatusControl: 
			case UHCICommandRegister:		return (uint32_t)__inw(port + r);
			case UHCIFrameListBaseAddress:	return (uint32_t)__inl(port + r);
			case UHCIStartOfFrameModify:	return (uint32_t)__inb(port + r);
			default:						{if((r - UHCIPortStatusControl) % sizeof(UHCIPortStatusControl_t) == 0){return __inw(port + r);}}
		}
	}
}

void UHCIWriteRegister(void *acpibase, uint32_t N, UHCIRegister r, uint32_t Data){
	uint32_t port = 0x00;
	if(GetUHCIControllerPort(acpibase, N, &port)){
		switch(r){
			case UHCIStatusRegister:
			case UHCIInterruptEnable:
			case UHCIFrameNumber:
			case UHCIPort1StatusControl: 
			case UHCICommandRegister:		__outw(port + r, Data);		break;
			case UHCIFrameListBaseAddress:	__outl(port + r, Data);		break;
			case UHCIStartOfFrameModify:	__outb(port + r, Data);		break;
			default:						{if((r - UHCIPortStatusControl) % sizeof(UHCIPortStatusControl_t) == 0){return __outw(port + r, Data);}}
		}
	}
}


bool GetUHCIControllerPort(void *acpibase, uint32_t n, uint32_t *port){
	if(port){
		PCIDevice *d = PCIeSearchDevice(acpibase, GetPCIstruct(SerialBus_USB_UHCI), n, NULL, NULL);
		if(d && d->HeaderType.Type == PCIGeneralDevice){
			PCIHeader0x0 *_0x0 = d;
			if(_0x0->BAR[4].IOBAR.IsIOSpace){
				*port = _0x0->BAR[4].IOBAR._4ByteAlignedAddress & ~0x3;
				return true;
			}
		}
	}
	return false;
}

//!	AI
void *UHCISearchDescriptor(void *acpibase, uint32_t N, bool H, uint32_t X, uint32_t Y, UHCIFrameListEntryType *Tptr){
    if(X >= 1024){return NULL;}

    //	Map the 1024-entry Frame List array
    uint32_t frame_list_phys = UHCIReadRegister(acpibase, N, UHCIFrameListBaseAddress);
    UHCIFrameListEntry_t *frame_list = (UHCIFrameListEntry_t *)MapVirtual(frame_list_phys);
    if(!frame_list){return NULL;}

    //	Read slot X from the Frame List
    UHCIFrameListEntry_t entry = frame_list[X];
    if(entry.EmptyFrame){return NULL;}

    uint32_t curr_phys = entry.PhysicalPointer << 4;
    UHCIFrameListEntryType current_type = (UHCIFrameListEntryType)entry.MemoryStructureType;

    //	Traverse coordinates safely
    while(curr_phys){
        void *virt_ptr = MapVirtual(curr_phys);
        if(!virt_ptr){return NULL;}
        if(X == 0 && Y == 0){
            if(Tptr){*Tptr = current_type;}
            return virt_ptr;
        }
        if(current_type == QueueHead){
            UHCIQueueHead_t *qh = (UHCIQueueHead_t *)virt_ptr;
            if(X && !qh->HorizontalPointer.EmptyFrame){
                curr_phys = qh->HorizontalPointer.PhysicalPointer << 4;
                current_type = (UHCIFrameListEntryType)qh->HorizontalPointer.MemoryStructureType;
                X--;
            }else if(Y && !qh->VerticalPointer.EmptyFrame){
                curr_phys = qh->VerticalPointer.PhysicalPointer << 4;
                current_type = (UHCIFrameListEntryType)qh->VerticalPointer.MemoryStructureType;
                Y--;
			//	Reached end of queue chain before target (X,Y)
            }else{return NULL;}
        }else{//	TransferDescriptor
            UHCITransferDescriptor_t *td = (UHCITransferDescriptor_t *)virt_ptr;
            if(Y&& !(td->NextDescriptor.NullEntry)){
                curr_phys = td->NextDescriptor.PhysicalPointer << 4;
                current_type = (UHCIFrameListEntryType)td->NextDescriptor.MemoryStructureType;
                Y--;
			//	Reached end of TD chain before target Y
            }else{return NULL;}
        }
    }
    return NULL;
}

bool UHCISendControlCommand(void *acpibase, uint32_t N, uint8_t dev_addr, USBControlRequest_t *req, void *data_buffer, bool is_low_speed){
    //	Locate pre-allocated Frame List and Queue Head
    uint32_t frame_list_phys = UHCIReadRegister(acpibase, N, UHCIFrameListBaseAddress);
    uint8_t *frame_list_virt = (uint8_t *)MapVirtual(frame_list_phys);
    if(!frame_list_virt){return false;}

    UHCIQueueHead_t *qh = (UHCIQueueHead_t *)frame_list_virt;

    //	Access static pre-allocated TD slots located directly after Queue Head in Frame List
    UHCITransferDescriptor_t *td_setup  = (UHCITransferDescriptor_t *)(frame_list_virt + sizeof(UHCIQueueHead_t)), 
							*td_data   = (UHCITransferDescriptor_t *)(frame_list_virt + sizeof(UHCIQueueHead_t) + sizeof(UHCITransferDescriptor_t)), 
							*td_status = (UHCITransferDescriptor_t *)(frame_list_virt + sizeof(UHCIQueueHead_t) + (2 * sizeof(UHCITransferDescriptor_t)));
    uint32_t phys_setup  = frame_list_phys + sizeof(UHCIQueueHead_t), 
			phys_data   = frame_list_phys + sizeof(UHCIQueueHead_t) + sizeof(UHCITransferDescriptor_t), 
			phys_status = frame_list_phys + sizeof(UHCIQueueHead_t) + (2 * sizeof(UHCITransferDescriptor_t)), 
			phys_req  = (uint32_t)(uintptr_t)MapPhysical(req), 
			phys_buf  = data_buffer ? (uint32_t)(uintptr_t)MapPhysical(data_buffer) : 0;

    //	Temporarily deactivate all stage descriptors to prevent race conditions with DMA
    td_setup->Status.Active  = false;
    td_data->Status.Active   = false;
    td_status->Status.Active = false;

    //	Update STATUS Stage TD
    td_status->NextDescriptor.NullEntry = true;
    td_status->Status.ErrorCounter = 3;
    td_status->Status.LowSpeed = is_low_speed;
    td_status->Status.InterruptOnComplete = true;
    td_status->PacketHeader.PacketType = (req->wLength <= 0 || ((req->bmRequestType & 0x80) == 0))? DeviceToHostTransferPacket: HostToDeviceTransferPacket;
    td_status->PacketHeader.Device = dev_addr & 0x7F;
    td_status->PacketHeader.Endpoint = 0;
    td_status->PacketHeader.DataToggle = 1;
    td_status->PacketHeader.MaximumLength = 0x7FF; // 0-byte payload
    td_status->PhysicalBufferAddress = 0;

    //	Update DATA Stage TD (if transfer has a data phase)
    if(req->wLength > 0){
        td_data->NextDescriptor.PhysicalPointer = phys_status >> 4;
        td_data->NextDescriptor.MemoryStructureType = TransferDescriptor;
        td_data->NextDescriptor.NullEntry = false;
        td_data->Status.ErrorCounter = 3;
        td_data->Status.LowSpeed = is_low_speed;
        td_data->PacketHeader.PacketType = (req->bmRequestType & 0x80) != 0? DeviceToHostTransferPacket: HostToDeviceTransferPacket;
        td_data->PacketHeader.Device = dev_addr & 0x7F;
        td_data->PacketHeader.Endpoint = 0;
        td_data->PacketHeader.DataToggle = 1;
        td_data->PacketHeader.MaximumLength = (req->wLength - 1) & 0x7FF;
        td_data->PhysicalBufferAddress = phys_buf;
    }

    //	Update SETUP Stage TD
    td_setup->NextDescriptor.PhysicalPointer = (req->wLength > 0? phys_data: phys_status) >> 4;
    td_setup->NextDescriptor.MemoryStructureType = TransferDescriptor;
    td_setup->NextDescriptor.NullEntry = false;
    td_setup->Status.ErrorCounter = 3;
    td_setup->Status.LowSpeed = is_low_speed;
    td_setup->PacketHeader.PacketType = SetupPacket;
    td_setup->PacketHeader.Device = dev_addr & 0x7F;
    td_setup->PacketHeader.Endpoint = 0;
    td_setup->PacketHeader.DataToggle = 0; // SETUP stage is always DATA0
    td_setup->PacketHeader.MaximumLength = 7; // Fixed 8 bytes (8 - 1)
    td_setup->PhysicalBufferAddress = phys_req;

    //	Ensure Queue Head points directly to SETUP stage TD
    qh->VerticalPointer.PhysicalPointer = phys_setup >> 4;
    qh->VerticalPointer.MemoryStructureType = TransferDescriptor;
    qh->VerticalPointer.EmptyFrame = false;

    //	Activate descriptors from Tail to Head (Status -> Data -> Setup)
    td_status->Status.Active = true;
    if(req->wLength > 0){td_data->Status.Active = true;}
    td_setup->Status.Active = true;

    //	Poll for execution completion on STATUS stage
    while(td_status->Status.Active){;}
    return (td_status->Status.Stalled == 0);
}

// void *UHCIGetDescriptor(void *acpibase, uint32_t N, uint8_t dev_addr, uint8_t config_index, USBGetDescriptorTypes gd, bool is_low_speed){
//     //	Request 9-byte Configuration Header
//     USBControlRequest_t req = {
// 		//	Device-to-Host | Standard | Device
//         .bmRequestType = 0x80, 
//         .bRequest = GET_DESCRIPTOR, 
// 		//	Type 0x02 (Configuration) | Index
//         .wValue = (((uint16_t)USBGetDescriptorCurrentSpeedConfiguration << 8) | config_index), 
//         .wIndex = 0x0000, 
//         .wLength = sizeof(USBGetDescriptorCurrentSpeedConfiguration_t) 
//     };
//     USBGetDescriptorCurrentSpeedConfiguration_t header = {0};

//     //	Issue header request via UHCISendControlCommand
// 	//	Transfer failed or stalled
//     if(!UHCISendControlCommand(acpibase, N, dev_addr, &req, &header, is_low_speed)){return NULL;}

//     //	Validate descriptor length and type
//     if(header.Header.DescriptorSize < sizeof(USBGetDescriptorCurrentSpeedConfiguration_t) || header.Header.DescriptorType != 0x02){return NULL; }

//     //	Allocate memory for the full configuration tree using wTotalLength
//     uint16_t total_len = header.TotalLength;
//     void *full_config = AllocateAlignedPages(NULL, total_len, ReadWritable | SupervisorMode, 0x00, 16);
//     if(!full_config){return NULL;}

//     // Update request to fetch the complete configuration payload
//     req.wLength = total_len;
// 	req.wValue = ((uint16_t)gd << 8) | config_index;
//     if(!UHCISendControlCommand(acpibase, N, dev_addr, &req, full_config, is_low_speed)){
//         FreePages(full_config);
//         return NULL;
//     }

// 	//	Returns buffer containing Configuration, Interface, and Endpoint structures
//     return full_config;
// }