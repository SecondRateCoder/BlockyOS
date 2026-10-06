#include "socket.h"
#include "sockets.h"
static const UINT64 nDrivers = 1;

socket_ret socketopen(UINT32 driver, UINT64 nARGbytes, ...){
	DEBUGPRINT(L"\nOpening Socket    Driver: %u  nARGS: %u", driver, nARGbytes);
	va_list args;
	va_start(args, nARGbytes);
	if(driver < nDrivers){
		UINT32 device = va_arg(args, UINT32);
		void *data = NULL;
		switch(driver){
			case 0: {
				socket_ret tmp = __froot_sckopen(device, nARGbytes - sizeof(UINT32), &args);
				data = __memdup(&tmp, sizeof(socket_ret));
				va_end(args);
				break;
			} default: {
				va_end(args);
				return (socket_ret){0};
			}
		}
		return (socket_ret){.errout = data? __noerr: __incompatible_arg, .data = data, .nData = (data? sizeof(socket_ret): 0)};
	}
	va_end(args);
	return socketret__noimpl;
}