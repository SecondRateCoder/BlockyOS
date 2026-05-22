#pragma once

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/tools/tools.h"
#include "socket.h"


extern socket_ret __attribute__((used, noinline, visibility("default"), optimize("O0"))) __fhandle_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...);
extern socket_ret *__attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi)) __froot_sckopen(UINT32 ignore, UINTN nARGbytes, va_list *args);
extern socket_ret __attribute__((used, noinline, visibility("default"), optimize("O0"))) __fhandle_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...);
extern socket_ret __attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi)) __fhandle_sckclose(socket_t * socket, UINTN nARGbytes, ...);
extern socket_ret __attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi)) __fhandle_sckOPENchild(socket_t * socket, UINTN nARGbytes, va_list args);
extern socket_ret __attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi)) __froot_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...);
extern socket_ret __attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi)) __froot_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...);
extern socket_ret __attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi)) __froot_sckclose(socket_t * socket, UINTN nARGbytes, ...);
extern socket_ret __attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi)) __froot_sckOPENchild(socket_t * socket, UINTN nARGbytes, ...);
extern socket_ret * __attribute__((used, noinline, visibility("default"), optimize("O0"), ms_abi)) __froot_sckopen(UINT32 ignore, UINTN nARGbytes, va_list *args);