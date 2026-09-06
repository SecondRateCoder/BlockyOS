#pragma once

#include "efi.h"
#include "efilib.h"

#include "Boot/UEFI/tools/tools.h"
#include "Boot/UEFI/drivers/.disk/fs/frat.h"
#include "Boot/UEFI/drivers/.disk/raw/raw.h"
#include "Boot/UEFI/drivers/crypto/blake2/ref/blake2.h"
#include "socket.h"


socket_ret socketfuncprefix __fhandle_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...);
socket_ret socketfuncprefix __fhandle_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...);
socket_ret socketfuncprefix __fhandle_sckclose(socket_t * socket, UINTN nARGbytes, ...);
socket_ret socketfuncprefix __fhandle_sckOPENchild(socket_t * socket, UINTN nARGbytes, va_list args);
socket_ret socketfuncprefix __fhandle_sckinfo(struct socket_t *socket, UINT32 Property, UINT32 subProperty);

socket_ret socketfuncprefix __froot_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...);
socket_ret socketfuncprefix __froot_sckopen(UINT32 ignore, UINTN nARGbytes, va_list *args);
socket_ret socketfuncprefix __froot_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...);
socket_ret socketfuncprefix __froot_sckclose(socket_t * socket, UINTN nARGbytes, ...);
socket_ret socketfuncprefix __froot_sckOPENchild(socket_t * socket, UINTN nARGbytes, ...);
socket_ret socketfuncprefix __froot_sckinfo(struct socket_t *socket, UINT32 Property, UINT32 subProperty);