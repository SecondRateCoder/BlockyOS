#include "kernel/services/peripheral/service.h"

#define PS2DataPort		0x60
#define PS2Status		0x64
#define PS2Command		0x64

#define PS2CommandOpcodeReadByte(N)	((uint8_t)(N) & 0x1F)	
enumdef(uint8_t, PS2CommandOpcode){
	//	Returns Controller Configuration Byte
	ReadControllerConfigurationByte = 0x20, 
	WriteControllerConfigurationByte = 0x60, 
	DisableSecondPS2Port = 0xA7, EnableSecondPS2Port = 0xA8, 
	//	0x00 test passed
	//	0x01 clock line stuck low
	//	0x02 clock line stuck high
	//	0x03 data line stuck low
	//	0x04 data line stuck high 
	TestSecondPS2Port = 0xA9, 
	//	 0x55 test passed
	//	0xFC test failed
	TestPS2Controller = 0xAA, 
	//	0x00 test passed
	//	0x01 clock line stuck low
	//	0x02 clock line stuck high
	//	0x03 data line stuck low
	//	0x04 data line stuck high 
	TestFirstPS2Port = 0xAB, 
	ReadInternalRAM = 0xAC, DiagnosticDump = ReadInternalRAM, 
	DisableSecondPS2Port = 0xAD, EnableSecondPS2Port = 0xAE, 
	ReadControllerInputPort = 0xC0, 
	CopyInputPortToStatusPortLow4 = 0xC1, 
	CopyInputPortToStatusPortHigh4 = 0xC2, 
	ReadControllerOutputPort = 0xD0, 
	WriteNextByteControllerOutputPort = 0xD1, 
	WriteNextBytePS2FirstOutputPort = 0xD2, 
	WriteNextBytePS2FirstOutputPort = 0xD3, 
	WriteNextBytePS2FirstInputPort = 0xD4, 
	WriteNextBytePS2FirstInputPort = 0xD5, 
	//	Pulse output line low for 6 ms. 
	//	Bits 0 to 3 are used as a mask 
	//		(0 = pulse line, 1 = don't pulse line) 
	//	and correspond to 4 different output lines.
};

typedef struct{
	uint8_t		DataPortClosed	: 1;
	uint8_t		InputPortsClosed: 1;
	uint8_t		SystemFlag		: 1;
	//	0 = data written to input buffer is data for PS/2 device.
	//	1 = data written to input buffer is data for PS/2 controller command.
	uint8_t		DataTarget		: 1;
	uint8_t		VendorSpecific0	: 1;
	uint8_t		VendorSpecific1	: 1;
	uint8_t		TimeoutError	: 1;
	uint8_t		ParityError		: 1;
}__packed PS2StatusRegister_t;

typedef struct{
	uint8_t		PS2Port1InterruptEnable		: 1;
	uint8_t		PS2Port2InterruptEnable		: 1;
	uint8_t		POSTComplete				: 1;
	uint8_t		Zero						: 1;
	uint8_t		PS2Port1ClockDisable		: 1;
	uint8_t		PS2Port2ClockDisable		: 1;
	uint8_t		PS2Port1TranslationEnable	: 1;
}__packed PS2ControllerConfigurationByte_t;

typedef struct{
}__packed PS2ControllerOutputPort_t;