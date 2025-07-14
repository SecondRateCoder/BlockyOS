#include "./src/Core/streams.h"


stream_t stream_init(char *Path, size_t Length, int TargetDevice[2], bool args[3], size_t BufferMultiplier){
	hcontext_t hc = (hcontext_t){
		.ID = (ID_t){
			.HeaderID = define_hid(),
			.ProcessID = define_pid()
		},
		.size = CONTEXTEXTRAS_SIZE,
		.addr = address_pointfree(DEFAULTSTART, Size),
		if(addr == INVALID_ADDR){return NULL;},
		.checks = CHECK_PROTECT,
		.flags = (hflags_t){
			false,
			false,
			is_kernelprotected(TargetDevice),
			true},
		.Extras = (uint8_t[CONTEXTEXTRAS_SIZE]){
			[0] = TargetDevice,
			[1] = concat_flags((bool[8])args),
			[2] = /*Device will handshake.*/
		}
	};
	Path[Length] = NULL;
	memcpy_unsafe(hc.Extras, hc.Extras[2], Path, Length);
	hc.Extras[hc.Extras[2]+Length] = Path[Length];
	hc.Extras[hc.Extras[2]+Length+1] /*= Will be handshaked.*/;
}

void stream_inc(ID_t ID){
	if(headerpeeksearch_unsafe(ID, HContextPeekerAttr_IsStream) == true){
		size_t addr = decode_size_t(headerpeeksearch_unsafe(ID, HContextPeekerAttr_Address), 0);
		if(addr != INVALID_ADDR){return NULL;}
		RAM[addr+3]+=1;
	}
}

void stream_dec(ID_t ID){
	if(headerpeeksearch_unsafe(ID, HContextPeekerAttr_IsStream) == true){
		size_t addr = decode_size_t(headerpeeksearch_unsafe(ID, HContextPeekerAttr_Address), 0);
		if(addr != INVALID_ADDR){return NULL;}
		RAM[addr+3]-=1;
	}
}

uint8_t stream_read(ID_t ID){
	if(headerpeeksearch_unsafe(ID, HContextPeekerAttr_IsStream) == true){
		size_t addr = decode_size_t(headerpeeksearch_unsafe(ID, HContextPeekerAttr_Address), 0);
		if(addr != INVALID_ADDR){return NULL;}
		return RAM[addr+3];
	}
	return NULL;
}

uint8_t concat_flags(bool flags[8]){
	uitn8_t out = 0;
	for(uint8_t cc =0; cc < 8;cc++){
		if(flags[cc] == NULL){continue;}
		switch(cc){
			case 0:	out|=STREAM_FLAG_READ_ACCESS; break;
			case 1:	out|=STREAM_FLAG_WRITE_ACCESS; break;
			case 2:	out|=STREAM_FLAG_UNSAFE; break;
			case 3:	out|=STREAM_FLAG_PROTECT; break;
			case 4:	out|=STREAM_FLAG_KERNEL_ONLY; break;
		}
	}
	return out;
}