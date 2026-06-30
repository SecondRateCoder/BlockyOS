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
#ifndef BLAKE2_IMPL_H
#define BLAKE2_IMPL_H

#include <efi.h>
// #include "src/Boot/UEFI/tools/tools.h"
#include "src/Boot/UEFI/tools/tools.h"

#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
	#if   defined(_MSC_VER)
		#define BLAKE2_INLINE __inline
	#elif defined(__GNUC__)
		#define BLAKE2_INLINE __inline__
	#else
		#define BLAKE2_INLINE
	#endif
#else
	#define BLAKE2_INLINE inline
#endif

static BLAKE2_INLINE UINT32 load32( const void *src )
{
#if defined(NATIVE_LITTLE_ENDIAN)
	UINT32 w;
	__memcpy(&w, src, sizeof w);
	return w;
#else
	const UINT8 *p = ( const UINT8 * )src;
	return (( UINT32 )( p[0] ) <<  0) |
				 (( UINT32 )( p[1] ) <<  8) |
				 (( UINT32 )( p[2] ) << 16) |
				 (( UINT32 )( p[3] ) << 24) ;
#endif
}

static BLAKE2_INLINE UINT64 load64( const void *src )
{
#if defined(NATIVE_LITTLE_ENDIAN)
	UINT64 w;
	__memcpy(&w, src, sizeof w);
	return w;
#else
	const UINT8 *p = ( const UINT8 * )src;
	return (( UINT64 )( p[0] ) <<  0) |
				 (( UINT64 )( p[1] ) <<  8) |
				 (( UINT64 )( p[2] ) << 16) |
				 (( UINT64 )( p[3] ) << 24) |
				 (( UINT64 )( p[4] ) << 32) |
				 (( UINT64 )( p[5] ) << 40) |
				 (( UINT64 )( p[6] ) << 48) |
				 (( UINT64 )( p[7] ) << 56) ;
#endif
}

static BLAKE2_INLINE UINT16 load16( const void *src )
{
#if defined(NATIVE_LITTLE_ENDIAN)
	UINT16 w;
	__memcpy(&w, src, sizeof w);
	return w;
#else
	const UINT8 *p = ( const UINT8 * )src;
	return ( UINT16 )((( UINT32 )( p[0] ) <<  0) |
											(( UINT32 )( p[1] ) <<  8));
#endif
}

static BLAKE2_INLINE void store16( void *dst, UINT16 w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
	__memcpy(dst, &w, sizeof w);
#else
	UINT8 *p = ( UINT8 * )dst;
	*p++ = ( UINT8 )w; w >>= 8;
	*p++ = ( UINT8 )w;
#endif
}

static BLAKE2_INLINE void store32( void *dst, UINT32 w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
	__memcpy(dst, &w, sizeof w);
#else
	UINT8 *p = ( UINT8 * )dst;
	p[0] = (UINT8)(w >>  0);
	p[1] = (UINT8)(w >>  8);
	p[2] = (UINT8)(w >> 16);
	p[3] = (UINT8)(w >> 24);
#endif
}

static BLAKE2_INLINE void store64( void *dst, UINT64 w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
	__memcpy(dst, &w, sizeof w);
#else
	UINT8 *p = ( UINT8 * )dst;
	p[0] = (UINT8)(w >>  0);
	p[1] = (UINT8)(w >>  8);
	p[2] = (UINT8)(w >> 16);
	p[3] = (UINT8)(w >> 24);
	p[4] = (UINT8)(w >> 32);
	p[5] = (UINT8)(w >> 40);
	p[6] = (UINT8)(w >> 48);
	p[7] = (UINT8)(w >> 56);
#endif
}

static BLAKE2_INLINE UINT64 load48( const void *src )
{
	const UINT8 *p = ( const UINT8 * )src;
	return (( UINT64 )( p[0] ) <<  0) |
				 (( UINT64 )( p[1] ) <<  8) |
				 (( UINT64 )( p[2] ) << 16) |
				 (( UINT64 )( p[3] ) << 24) |
				 (( UINT64 )( p[4] ) << 32) |
				 (( UINT64 )( p[5] ) << 40) ;
}

static BLAKE2_INLINE void store48( void *dst, UINT64 w )
{
	UINT8 *p = ( UINT8 * )dst;
	p[0] = (UINT8)(w >>  0);
	p[1] = (UINT8)(w >>  8);
	p[2] = (UINT8)(w >> 16);
	p[3] = (UINT8)(w >> 24);
	p[4] = (UINT8)(w >> 32);
	p[5] = (UINT8)(w >> 40);
}

static BLAKE2_INLINE UINT32 rotr32( const UINT32 w, const unsigned c )
{
	return ( w >> c ) | ( w << ( 32 - c ) );
}

static BLAKE2_INLINE UINT64 rotr64( const UINT64 w, const unsigned c )
{
	return ( w >> c ) | ( w << ( 64 - c ) );
}

/* prevents compiler optimizing out __memset() */
static BLAKE2_INLINE void secure_zero_memory(void *v, UINT64 n)
{
  static void (*const volatile memset_v)(void *, UINT8, UINT64) = &__memset;
  memset_v(v, 0, n);
}

#endif
