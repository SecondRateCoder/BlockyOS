#include "./src/Core/RAM/memtypes.h"
#include <./src/Core/General.h>

#pragma region Alloca
void *alloca(size_t Size, uint8_t ProcessID[IDSize]){
	size_t freeaddr = address_pointfree(RAMMeta, Size);
	if(freeaddr == INVALID_ADDR){
		/*Define Virt-RAM in File-system.*/
	}
	else{memcpy_unsafe(RAM, freeaddr, (uint8_t *){0}, Size);}
	headerMeta_store((hcontext_t){
		.addr = freeaddr,
		.checks = CHECK_PROTECT,
		.flags = (hflags_t){
			.IsKernel = (ProcessID == KERNEL_ID),
			.IsPrivate = true,
			.IsProcess = false, 
			.IsStream = false,
			.IsThread = false
		},
		.ID = (ID_t){
			.ProcessID = ProcessID,
			.HeaderID = define_hid()
		},
		.size = Size
	});
}

bool is_alloca(void *data){
	
}
#pragma endregion

#pragma region De-Alloca
void dealloca_unsafe(void *header){
	/*
		Get the pointer to header,
		find the Metadata location where it lies or is within,
		Clear out the HeaderMetadata.
	*/
	size_t temp = RAMMeta, addr = header;
	while(temp < _ram_end){
		if(decode_size_t(RAM[temp+(sizeof(ID_t)+sizeof(size_t))], 0) == addr){
			memcpy_unsafe(RAM, temp, (uint8_t[context_size]){0}, context_size);
			return;
		}
		temp+=context_size;
	}
}

#pragma endregion