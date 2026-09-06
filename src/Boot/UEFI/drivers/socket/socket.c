#include "socket.h"
#include "sockets.h"
static const UINTN nDrivers = 1;

socket_ret socketopen(UINT32 driver, UINTN nARGbytes, ...){
	DEBUGPRINT(L"\nOpening Socket    Driver: %u  nARGS: %u", driver, nARGbytes);
	va_list args;
	va_start(args, nARGbytes);
	if((driver < nDrivers)){
		UINT32 device = va_arg(args, UINT32);
		void *data = NULL;
		switch(driver){
			case 0: {
				data = (__froot_sckopen(device, nARGbytes - sizeof(UINT32), &args)).data;
				break;
			} default: {
				va_end(args);
				return (socket_ret){0};
			}
		}
		socket_ret out = {
			.errout = __noerr,
			.data = data,
			.nData = sizeof(socket_ret)
		};
		if(out.data){out.errout = __noerr;}else{out.errout = __incompatible_arg;		out.nData = 0;}
		return out;
	}
	va_end(args);
	return socketret__noimpl;
}