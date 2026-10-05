#pragma once

#include "efi.h"
#include "efilib.h"

#include "Boot/UEFI/tools/tools.h"

#define socketfuncprefix// __attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi))


enumdef(UINT16, socket_retFLAG){
	__noerr = 0x0, 
	__incompatible_arg = 0x1, 
	__noimpl_socketfunc = 0x2, 
	__noexist = 0x4, 
	__undeferr = 0x8, 
};

#define socketreterr(ret, ndata)	(!(((ret).errout == __noerr) && (ret).data && ((ret).nData == (ndata))))
typedef struct socket_ret{
	socket_retFLAG errout;
	UINT64 nData;
	void *data;
}socket_ret;

#define socketret_noerr_empty   	(socket_ret){__noerr, 0, NULL}
#define socketret__noimpl       	(socket_ret){__noimpl_socketfunc, 0, NULL}

// Helper macro to expand the 17th argument passed to it
#define COUNT_ARGS_IMPL( _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, _11, _12, _13, _14, _15, _16, N, ...)	N
// Public macro to count up to 16 variadic arguments
#define COUNT_ARGS(...)		COUNT_ARGS_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define socketfarg1(arg, ...)		sizeof(arg)
#define socketfarg2(arg, ...)		sizeof(arg) + socketfarg1(__VA_ARGS__)
#define socketfarg3(arg, ...)		sizeof(arg) + socketfarg2(__VA_ARGS__)
#define socketfarg4(arg, ...)		sizeof(arg) + socketfarg3(__VA_ARGS__)
#define socketfarg5(arg, ...)		sizeof(arg) + socketfarg4(__VA_ARGS__)
#define socketfarg6(arg, ...)		sizeof(arg) + socketfarg5(__VA_ARGS__)
#define socketfarg7(arg, ...)		sizeof(arg) + socketfarg6(__VA_ARGS__)
#define socketfarg8(arg, ...)		sizeof(arg) + socketfarg7(__VA_ARGS__)
#define socketfarg9(arg, ...)		sizeof(arg) + socketfarg8(__VA_ARGS__)
#define socketfarg10(arg, ...)		sizeof(arg) + socketfarg9(__VA_ARGS__)
#define socketfarg11(arg, ...)		sizeof(arg) + socketfarg10(__VA_ARGS__)
#define socketfarg12(arg, ...)		sizeof(arg) + socketfarg11(__VA_ARGS__)
#define socketfarg13(arg, ...)		sizeof(arg) + socketfarg12(__VA_ARGS__)
#define socketfarg14(arg, ...)		sizeof(arg) + socketfarg13(__VA_ARGS__)
#define socketfarg15(arg, ...)		sizeof(arg) + socketfarg14(__VA_ARGS__)
#define socketfarg16(arg, ...)		sizeof(arg) socketfarg15(__VA_ARGS__)

#define socketfargcombine(n)socketfarg ## n
#define socketfcall(s, f, ...)		s->f(s, __VA_ARG_NSUFFIX__(socketfarg, __VA_ARGS__)(__VA_ARGS__), __VA_ARGS__)

/// @brief Standardised, opens a Socket.
socket_ret socketopen(UINT32 driver, UINT64 nARGbytes, ...);

struct socket_t;

/// @brief Unique to each driver
typedef volatile socket_ret (socketfuncprefix *socketOPEN)(UINT32 device, UINT64 nARGbytes, va_list *args);
typedef volatile socket_ret (socketfuncprefix *socketINFO)(struct socket_t *socket, UINT64 nARGbytes, UINT32 Property, UINT32 subProperty, ...);

/// @brief Unique to each socket.
typedef volatile socket_ret (socketfuncprefix *socketOPENchild)(struct socket_t *socket, UINT64 nARGbytes, ...);
typedef volatile socket_ret (socketfuncprefix *socketCLOSE)(struct socket_t *socket, UINT64 nARGbytes, ...);
typedef volatile socket_ret (socketfuncprefix *socketREADraw)(struct socket_t *socket, UINT64 nARGbytes, UINT64 posBYTES, UINT64 readBYTES, ...);
typedef volatile socket_ret (socketfuncprefix *socketWRITEraw)(struct socket_t *socket, UINT64 nARGbytes, void *data, UINT64 posBYTES, UINT64 nBYTES, ...);
typedef socketREADraw socketREAD;
typedef socketWRITEraw socketWRITE;

typedef struct socket_t{
	void *persistent;
	socketINFO info;
	socketREAD read;
	socketWRITE write;
	struct raw{
		socketREADraw read;
		socketWRITEraw write;
	}raw;
	socketOPENchild open;
	socketCLOSE close;
}socket_t;