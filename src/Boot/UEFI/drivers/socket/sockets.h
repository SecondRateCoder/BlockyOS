#pragma once

#include "efi.h"
#include "efilib.h"

#include "socket.h"

#include "Boot/UEFI/standard.h"
#include "Boot/UEFI/tools/tools.h"
#include "Boot/UEFI/drivers/.disk/fs/frat.h"
#include "Boot/UEFI/drivers/.disk/raw/raw.h"
#include "Boot/UEFI/drivers/crypto/blake2/ref/blake2.h"


socket_ret socketfuncprefix __fhandle_sckwrite(socket_t * socket, UINT64 nARGbytes, void *data, UINT64 posBYTES, UINT64 nBYTES, ...);
socket_ret socketfuncprefix __fhandle_sckread(socket_t * socket, UINT64 nARGbytes, UINT64 posBYTES, UINT64 readBYTES, ...);
socket_ret socketfuncprefix __fhandle_sckclose(socket_t * socket, UINT64 nARGbytes, ...);
socket_ret socketfuncprefix __fhandle_sckOPENchild(socket_t * socket, UINT64 nARGbytes, va_list args);
socket_ret socketfuncprefix __fhandle_sckinfo(struct socket_t *socket, UINT64 nArgBytes, UINT32 Property, UINT32 subProperty, ...);

socket_ret socketfuncprefix __froot_sckread(socket_t * socket, UINT64 nARGbytes, UINT64 posBYTES, UINT64 readBYTES, ...);
socket_ret socketfuncprefix __froot_sckopen(UINT32 ignore, UINT64 nARGbytes, va_list *args);
socket_ret socketfuncprefix __froot_sckwrite(socket_t * socket, UINT64 nARGbytes, void *data, UINT64 posBYTES, UINT64 nBYTES, ...);
socket_ret socketfuncprefix __froot_sckclose(socket_t * socket, UINT64 nARGbytes, ...);
socket_ret socketfuncprefix __froot_sckOPENchild(socket_t * socket, UINT64 nARGbytes, ...);
socket_ret socketfuncprefix __froot_sckinfo(struct socket_t *socket, UINT64 nArgBytes, UINT32 Property, UINT32 subProperty, ...);