#include "./src/Core/Devices/PCI.h"
#include "./src/Core/Devices/PCIe.h"

#define capal_t capabilities_list
typedef struct capabilities_list{
	//if true: disable: (), enable: (table_offset, pending_bitoffset, BIR, pendingbit_BIR, message_control, next_offset, capabilities_id)
	//if false: disable: (), enable: (message_control, next_offset, capabilities_id, messageaddress_low, messageaddress_high, message_data, _mask_, _pending_)
	bool is_msix;
	//BIR(MSIX): What BAR is used for this specific capability in the parent device, e.g  BIR: 0, BAR: 0
	uint8_t next_offset, capabilities_id, table_offset[3], pending_bitoffset[3], BIR, pendingbit_BIR;
	uint16_t message_control, message_data;
	uint32_t messageaddress_low, messageaddress_high, _mask_, _pending_;
}capabilities_list;

#pragma region MSI
#define MSI_PER_VECTORMASKING(CR) is_set((size_t)cr, 8)
#define MSI_BIT64(CR) is_set((size_t)CR, 7)
#define MSI_MULTIMESSAGE_ENABLE(CR) (uint8_t[3]){is_set((size_t)CR, 6), is_set((size_t)CR, 5), is_set((size_t)CR, 4)}
#define MSI_MULIMESSAGE_CAPABLE(CR) (uint8_t[3]){is_set((size_t)CR, 3), is_set((size_t)CR, 2), is_set((size_t)CR, 1)}
#define MSI_ENABLE is_set(CR) ((size_t)CR, 0)
#define MSI_CAPABILITIESLIST_ID 0x05
#pragma endregion



#pragma region MSI_X
#pragma region MESSAGE CONTROL BITS
#define ENABLE(M) is_set((size_t)M, 15)
#define FUNCTION_MASK(M) is_set((size_t)M, 14)
//number of entries in the table.
#define TABLE_SIZE(M) (uint8_t[11]){is_set((size_t)M, 10), is_set((size_t)M, 9), is_set((size_t)M, 8), \
is_set((size_t)M, 7), is_set((size_t)M, 6), is_set((size_t)M, 5), is_set((size_t)M, 4), \
is_set((size_t)M, 3), is_set((size_t)M, 2), is_set((size_t)M, 1), is_set((size_t)M, 0)}

#define VECTOR_CONTROL(MU, ML) ((uint32_t)((MU) >> 32))
#define MESSAGE_DATA(MU, ML) ((uint32_t)((MU) & BIT32_MASK))
#define MESSAGEADDRESS_HIGH(MU, ML) ((uint32_t)((ML) >> 32))
#define MESSAGEADDRESS_LOW(MU, ML) ((uint32_t)((ML) & BIT32_MASK))
#define MSIX_CAPABILITIESLIST_ID 0x11
/*
Capabilities Pointer: Read the Capabilities Pointer register at offset 0x34 in the device's PCI Configuration Space. This points to the first capability structure.

Capability List: Traverse the linked list of capabilities by reading the Next Capability Pointer,
 in each structure until you find the one with a Capability ID of 0x11 (which is the MSI-X Capability ID)
*/
#pragma endregion
#pragma endregion


/*
Getting the Capabilities List, get the Capabilities Pointer, use it as an Offset and read the value.
This is

*/
size_t capabilities_readlist(const device_t *dev){
	enableaddr_read(*dev);
	capal_t capabilities;
    uint8_t capabilities_pointer = dev->meta->capabilities_pointer;
	uint8_t capabilities_id = dev->meta->capability_id;
    if(STATUSBITS_CAPABILITIES(dev->meta->status) == true){return NULL;}
    while(capabilities_pointer != 0x00){
        uint16_t value = uintconv32_16(device_read32(*dev, capabilities_pointer));
		uint32_t list[6] =(capal_t[6]){
			[0] = device_read32(*dev, capabilities_pointer),
			[1] = device_read32(*dev, capabilities_pointer+REG1OFFSET),
			[2] =device_read32(*dev, capabilities_pointer+REG2OFFSET),
			[3] =device_read32(*dev, capabilities_pointer+REG3OFFSET),
			[4] =device_read32(*dev, capabilities_pointer+REG4OFFSET),
			[5] =device_read32(*dev, capabilities_pointer+REG5OFFSET),
		};
		switch(capabilities_id){
			case MSI_CAPABILITIESLIST_ID:
				if(list[0] & BIT8_MASK != 0x0){
					capabilities =(capal_t){
						.is_msix = false,
						.message_control= (uint16_t)((list[0] >> 16) & BIT16_MASK),
						.next_offset= (uint8_t)((list[0] >> 8) & BIT8_MASK),
						.capabilities_id = (uint8_t)(list[0] & BIT8_MASK),
						.messageaddress_low = list[1],
						.messageaddress_high = list[2],
						.message_data = list[3] & BIT8_MASK,
						._mask_ =list[4],
						._pending_ = list[5]
					};
					capabilities_id = capabilities.capabilities_id;
				}
				break;

			case MSIX_CAPABILITIESLIST_ID:
				capabilities = (capal_t){
					.is_msix =true,
					.message_control = (uint16_t)((list[0] >> 16) & BIT16_MASK),
					.capabilities_id =list[0]  & BIT8_MASK,
					.next_offset = (uint8_t)((list[0] >> 8)& BIT8_MASK),
					.table_offset = (uint8_t[3]){((list[1] >> 24)& BIT8_MASK), ((list[1] >> 16)& BIT8_MASK), ((list[1] >> 8)& BIT8_MASK)},
					.BIR = (uint8_t)(list[1] & BIT2_MASK),
					.pending_bitoffset = (uint8_t[3]){((list[2] >> 24)& BIT8_MASK), ((list[2] >> 16)& BIT8_MASK), ((list[2] >> 8)& BIT8_MASK)},
					.pendingbit_BIR = list[2] & BIT2_MASK,
				};
				capabilities_id = capabilities.capabilities_id;
				break;
		}
    }
}