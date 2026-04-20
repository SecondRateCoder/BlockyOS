#include "standard.h"

typedef struct eflags32{
	uint32_t Carry		: 1;
	uint32_t Parity		: 1;
	uint32_t Aux		: 1;
	uint32_t Zero		: 1;
	uint32_t Sign		: 1;
	uint32_t Trap		: 1;
	uint32_t InterruptEn: 1;
	uint32_t Direction	: 1;
	uint32_t Overflow	: 1;
	uint32_t IOPrivilege: 1;
	uint32_t NestedTask	: 1;
	uint32_t Resume		: 1;
	uint32_t Virt8086	: 1;
	uint32_t Alignment	: 1;
	uint32_t VirtIntr	: 1;
	uint32_t VirtIntrPnd: 1;
	uint32_t CPUIDEnable: 1;
}PACKEDSTRUCT eflags32;

typedef struct cr032{
	uint32_t ProtectedModeEnable		: 1;
	uint32_t MonitorCoProc				: 1;
	uint32_t x87FPUEmu					: 1;
	uint32_t TaskSwitch					: 1;
	uint32_t ExtType					: 1;
	uint32_t NumError					: 1;
	uint32_t reserved					: 11;
	uint32_t WriteProtect				: 1;
	uint32_t reserved					: 1;
	uint32_t No_WriteThrough			: 1;
	uint32_t CacheDisable				: 1;
	uint32_t PagingEnable				: 1;
}PACKEDSTRUCT cr032;

typedef struct cr332{
	uint32_t reserved				: 4;
	uint32_t PageLvl_WriteThrough	: 1;
	uint32_t PageLvl_CacheDisable	: 6;
	uint32_t reserved				: 6;
	uint32_t PageDir_BaseAddress	; 19;
}PACKEDSTRUCT cr332;

typedef struct cr432{
	
}cr432;
