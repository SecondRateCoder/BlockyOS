#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include "ref/blake2.h"
#include <varargs.h>

typedef struct socket_ret{
    uint16_t errout;
    size_t nData;
    void *data;
}socket_ret;

/// @brief Standardised, opens a Socket.
typedef __cdecl socket_ret *socketOPEN(uint32_t devicecode, size_t nARGbytes, ...);

/// @brief Unique to each socket.
typedef __cdecl socket_ret *(*socketREADraw)(struct socket_t *socket, size_t readBYTES, size_t nARGbytes, ...);
typedef __cdecl socket_ret *(*socketWRITEraw)(struct socket_t *socket, void *data, size_t nBYTES, size_t nARGbytes, ...);
typedef __cdecl socket_ret *(*socketREAD)(struct socket_t *socket, size_t readBYTES, size_t nARGbytes, ...);
typedef __cdecl socket_ret *(*socketWRITE)(struct socket_t *socket, void *data, size_t nBYTES, size_t nARGbytes, ...);
typedef __cdecl socket_ret *(*socketOPENchild)(struct socket_t *socket, size_t nARGbytes, ...);
typedef __cdecl socket_ret *(*socketCLOSE)(struct socket_t *socket, size_t nARGbytes, ...);

typedef struct socket_t{
    socketCLOSE lclose;
    socketREAD read;
    socketWRITE write;
    struct raw{
        socketREADraw read;
        socketWRITEraw write;
    }raw;
    socketOPENchild open;
    socketCLOSE close;
}socket_t;