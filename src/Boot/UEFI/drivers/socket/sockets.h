#pragma once

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/tools/tools.h"
#include "src/Boot/UEFI/drivers/.disk/fs/frat.h"
#include "src/Boot/UEFI/drivers/.disk/raw/raw.h"
#include "src/Boot/UEFI/drivers/crypto/blake2/ref/blake2.h"
#include "socket.h"


extern socket_ret socketfuncprefix __fhandle_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...);
extern socket_ret socketfuncprefix __fhandle_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...);
extern socket_ret socketfuncprefix __fhandle_sckclose(socket_t * socket, UINTN nARGbytes, ...);
extern socket_ret socketfuncprefix __fhandle_sckOPENchild(socket_t * socket, UINTN nARGbytes, va_list args);

extern socket_ret socketfuncprefix __froot_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...);
extern socket_ret *socketfuncprefix __froot_sckopen(UINT32 ignore, UINTN nARGbytes, va_list *args);
extern socket_ret socketfuncprefix __froot_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...);
extern socket_ret socketfuncprefix __froot_sckclose(socket_t * socket, UINTN nARGbytes, ...);
extern socket_ret socketfuncprefix __froot_sckOPENchild(socket_t * socket, UINTN nARGbytes, ...);