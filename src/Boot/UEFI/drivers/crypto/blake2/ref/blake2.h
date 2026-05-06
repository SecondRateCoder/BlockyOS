/*
	 BLAKE2 reference source code package - reference C implementations

	 Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.  You may use this under the
	 terms of the CC0, the OpenSSL Licence, or the Apache Public License 2.0, at
	 your option.  The terms of these licenses can be found at:

	 - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
	 - OpenSSL license   : https://www.openssl.org/source/license.html
	 - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0

	 More information about the BLAKE2 hash function can be found at
	 https://blake2.net.
*/
#ifndef BLAKE2_H
#define BLAKE2_H

#include <efi.h>

#if defined(_MSC_VER)
#define BLAKE2_PACKED(x) __pragma(pack(push, 1)) x __pragma(pack(pop))
#else
#define BLAKE2_PACKED(x) x __attribute__((packed))
#endif

#if defined(__cplusplus)
extern "C" {
#endif

	enum blake2s_constant
	{
		BLAKE2S_BLOCKBYTES = 64,
		BLAKE2S_OUTBYTES   = 32,
		BLAKE2S_KEYBYTES   = 32,
		BLAKE2S_SALTBYTES  = 8,
		BLAKE2S_PERSONALBYTES = 8
	};

	enum blake2b_constant
	{
		BLAKE2B_BLOCKBYTES = 128,
		BLAKE2B_OUTBYTES   = 64,
		BLAKE2B_KEYBYTES   = 64,
		BLAKE2B_SALTBYTES  = 16,
		BLAKE2B_PERSONALBYTES = 16
	};

	typedef struct blake2s_state__
	{
		UINT32 h[8];
		UINT32 t[2];
		UINT32 f[2];
		UINT8  buf[BLAKE2S_BLOCKBYTES];
		UINT64   buflen;
		UINT64   outlen;
		UINT8  last_node;
	} blake2s_state;

	typedef struct blake2b_state__
	{
		UINT64 h[8];
		UINT64 t[2];
		UINT64 f[2];
		UINT8  buf[BLAKE2B_BLOCKBYTES];
		UINT64   buflen;
		UINT64   outlen;
		UINT8  last_node;
	} blake2b_state;

	typedef struct blake2sp_state__
	{
		blake2s_state S[8][1];
		blake2s_state R[1];
		UINT8       buf[8 * BLAKE2S_BLOCKBYTES];
		UINT64        buflen;
		UINT64        outlen;
	} blake2sp_state;

	typedef struct blake2bp_state__
	{
		blake2b_state S[4][1];
		blake2b_state R[1];
		UINT8       buf[4 * BLAKE2B_BLOCKBYTES];
		UINT64        buflen;
		UINT64        outlen;
	} blake2bp_state;


	BLAKE2_PACKED(struct blake2s_param__
	{
		UINT8  digest_length; /* 1 */
		UINT8  key_length;    /* 2 */
		UINT8  fanout;        /* 3 */
		UINT8  depth;         /* 4 */
		UINT32 leaf_length;   /* 8 */
		UINT32 node_offset;  /* 12 */
		UINT16 xof_length;    /* 14 */
		UINT8  node_depth;    /* 15 */
		UINT8  inner_length;  /* 16 */
		/* UINT8  reserved[0]; */
		UINT8  salt[BLAKE2S_SALTBYTES]; /* 24 */
		UINT8  personal[BLAKE2S_PERSONALBYTES];  /* 32 */
	});

	typedef struct blake2s_param__ blake2s_param;

	BLAKE2_PACKED(struct blake2b_param__
	{
		UINT8  digest_length; /* 1 */
		UINT8  key_length;    /* 2 */
		UINT8  fanout;        /* 3 */
		UINT8  depth;         /* 4 */
		UINT32 leaf_length;   /* 8 */
		UINT32 node_offset;   /* 12 */
		UINT32 xof_length;    /* 16 */
		UINT8  node_depth;    /* 17 */
		UINT8  inner_length;  /* 18 */
		UINT8  reserved[14];  /* 32 */
		UINT8  salt[BLAKE2B_SALTBYTES]; /* 48 */
		UINT8  personal[BLAKE2B_PERSONALBYTES];  /* 64 */
	});

	typedef struct blake2b_param__ blake2b_param;

	typedef struct blake2xs_state__
	{
		blake2s_state S[1];
		blake2s_param P[1];
	} blake2xs_state;

	typedef struct blake2xb_state__
	{
		blake2b_state S[1];
		blake2b_param P[1];
	} blake2xb_state;

	/* Padded structs result in a compile-time error */
	enum {
		BLAKE2_DUMMY_1 = 1/(int)(sizeof(blake2s_param) == BLAKE2S_OUTBYTES),
		BLAKE2_DUMMY_2 = 1/(int)(sizeof(blake2b_param) == BLAKE2B_OUTBYTES)
	};

	/* Streaming API */
	int blake2s_init( blake2s_state *S, UINT64 outlen );
	int blake2s_init_key( blake2s_state *S, UINT64 outlen, const void *key, UINT64 keylen );
	int blake2s_init_param( blake2s_state *S, const blake2s_param *P );
	int blake2s_update( blake2s_state *S, const void *in, UINT64 inlen );
	int blake2s_final( blake2s_state *S, void *out, UINT64 outlen );

	int blake2b_init( blake2b_state *S, UINT64 outlen );
	int blake2b_init_key( blake2b_state *S, UINT64 outlen, const void *key, UINT64 keylen );
	int blake2b_init_param( blake2b_state *S, const blake2b_param *P );
	int blake2b_update( blake2b_state *S, const void *in, UINT64 inlen );
	int blake2b_final( blake2b_state *S, void *out, UINT64 outlen );

	int blake2sp_init( blake2sp_state *S, UINT64 outlen );
	int blake2sp_init_key( blake2sp_state *S, UINT64 outlen, const void *key, UINT64 keylen );
	int blake2sp_update( blake2sp_state *S, const void *in, UINT64 inlen );
	int blake2sp_final( blake2sp_state *S, void *out, UINT64 outlen );

	int blake2bp_init( blake2bp_state *S, UINT64 outlen );
	int blake2bp_init_key( blake2bp_state *S, UINT64 outlen, const void *key, UINT64 keylen );
	int blake2bp_update( blake2bp_state *S, const void *in, UINT64 inlen );
	int blake2bp_final( blake2bp_state *S, void *out, UINT64 outlen );

	/* Variable output length API */
	int blake2xs_init( blake2xs_state *S, const UINT64 outlen );
	int blake2xs_init_key( blake2xs_state *S, const UINT64 outlen, const void *key, UINT64 keylen );
	int blake2xs_update( blake2xs_state *S, const void *in, UINT64 inlen );
	int blake2xs_final(blake2xs_state *S, void *out, UINT64 outlen);

	int blake2xb_init( blake2xb_state *S, const UINT64 outlen );
	int blake2xb_init_key( blake2xb_state *S, const UINT64 outlen, const void *key, UINT64 keylen );
	int blake2xb_update( blake2xb_state *S, const void *in, UINT64 inlen );
	int blake2xb_final(blake2xb_state *S, void *out, UINT64 outlen);

	/* Simple API */
	int blake2s( void *out, UINT64 outlen, const void *in, UINT64 inlen, const void *key, UINT64 keylen );
	int blake2b( void *out, UINT64 outlen, const void *in, UINT64 inlen, const void *key, UINT64 keylen );

	int blake2sp( void *out, UINT64 outlen, const void *in, UINT64 inlen, const void *key, UINT64 keylen );
	int blake2bp( void *out, UINT64 outlen, const void *in, UINT64 inlen, const void *key, UINT64 keylen );

	int blake2xs( void *out, UINT64 outlen, const void *in, UINT64 inlen, const void *key, UINT64 keylen );
	int blake2xb( void *out, UINT64 outlen, const void *in, UINT64 inlen, const void *key, UINT64 keylen );

	/* This is simply an alias for blake2b */
	int blake2( void *out, UINT64 outlen, const void *in, UINT64 inlen, const void *key, UINT64 keylen );

#if defined(__cplusplus)
}
#endif

#endif
