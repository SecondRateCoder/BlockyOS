#pragma once

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/standard.h"
#include "src/Boot/UEFI/tools/tools.h"
#include "src/Boot/UEFI/drivers/.disk/fs/frat.h"
#include "src/Boot/UEFI/drivers/.disk/raw/raw.h"
#include "src/Boot/UEFI/drivers/crypto/blake2/ref/blake2.h"

struct socket_t;

enumdef(socket_retFLAG, uint16_t){
	__noerr = 0x0,
	__incompatible_arg = 0x1,
	__noimpl_socketfunc = 0x2,
	__noexist = 0x4
};

#define socketreterr(ret, ndata)	(!(((ret).errout == __noerr) && (ret).data && ((ret).nData == (ndata))))
typedef struct socket_ret{
	socket_retFLAG errout;
	UINTN nData;
	void *data;
}socket_ret;

#define socketret_noerr_empty   (socket_ret){__noerr, 0, NULL}
#define socketret__noimpl       (socket_ret){__noimpl_socketfunc, 0, NULL}
#define socketfunc_noimpldef(name) socket_ret name(socket_t *socket, UINTN readBYTES, UINTN nARGbytes, ...){return (socket_ret)socketret__noimpl;}

/// @brief Standardised, opens a Socket.
socket_ret socketopen(UINT32 driver, UINTN nARGbytes, ...);

/// @brief Unique to each driver
typedef socket_ret *(*socketOPEN)(UINT32 device, UINTN nARGbytes, va_list *args);

/// @brief Unique to each socket.
typedef socket_ret (*socketOPENchild)(struct socket_t * socket, UINTN nARGbytes, ...);
typedef socket_ret (*socketCLOSE)(struct socket_t * socket, UINTN nARGbytes, ...);
typedef socket_ret (*socketREADraw)(struct socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...);
typedef socket_ret (*socketWRITEraw)(struct socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...);
typedef socketREADraw socketREAD;
typedef socketWRITEraw socketWRITE;

typedef struct socket_t{
	void *persistent;
	socketREAD read;
	socketWRITE write;
	struct raw{
		socketREADraw read;
		socketWRITEraw write;
	}raw;
	socketOPENchild open;
	socketCLOSE close;
}socket_t;