#define _STDINT_H
#include <uefi/uefi.h>

#define FORCE_SYMBOLEXPOSURE EXTERNALISESYMBOL EMITTABLESYMBOL
#define EXTERNALISESYMBOL __attribute__((extermally_visible))
#define EMITTABLESYMBOL __attribute__((used))
#define PACKEDSTRUCT __attribute__((packed))
#define ASMCALL __attribute__((cdecl))
#define LINKERSECTION(SECTION) __attribute__((section(SECTION)))
#define LINKERSECTIONEXT(SECTION, ALIGNMENT, FLAGS) __attribute__((section(SECTION), align(ALIGNMENT), FLAGS))
#define INTERRUPTCALL __attribute__((interrupt))
#define STACKLESSCALL __attribute__((naked))

#define FLAGSET(INT, FLAG) (INT |= FLAG)
#define FLAGUNSET(INT, FLAG) (INT |= ~FLAG)
#define FLAGTOGGLE(INT, FLAG) (INT ^ FLAG)
#define FLAGCHECK(INT, FLAG) ((INT & FLAG) == FLAG)

#define NULLSTR(s) (char[s]){0}
#define defenum(type, name) typedef type name; enum

#ifndef _STDINT_H
#define _STDINT_H

/* Prefer the system header when available */
#if defined(__has_include)
#  if __has_include(<stdint.h>)
#    include <stdint.h>
#  endif
#endif

/* If stdint.h wasn't provided, provide minimal fallbacks */
#ifndef UINT32_MAX
#include <limits.h>

/* 8-byte detection helpers */
#if defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 8)
  /* long is 64-bit (LP64) */
  typedef signed char         int8_t;
  typedef unsigned char       uint8_t;
  typedef short               int16_t;
  typedef unsigned short      uint16_t;
  typedef int                 int32_t;
  typedef unsigned int        uint32_t;
  typedef long long           int64_t;
  typedef unsigned long       uint64_t;
  typedef unsigned long       uintptr_t;
#elif defined(__SIZEOF_LONG_LONG__) && (__SIZEOF_LONG_LONG__ == 8)
  /* long long is 64-bit (LLP64 or other) */
  typedef signed char         int8_t;
  typedef unsigned char       uint8_t;
  typedef short               int16_t;
  typedef unsigned short      uint16_t;
  typedef int                 int32_t;
  typedef unsigned int        uint32_t;
  typedef long long           int64_t;
  typedef unsigned long long  uint64_t;
  /* choose uintptr_t by pointer size if available */
  #if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
    typedef unsigned long long uintptr_t;
  #elif defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 8)
    typedef unsigned long       uintptr_t;
  #else
    typedef unsigned int        uintptr_t; /* fallback for 32-bit */
  #endif
#else
  /* Fallback using limits.h constants */
  #if (ULONG_MAX == 18446744073709551615UL)
    typedef long long           int64_t;
    typedef unsigned long       uint64_t;
    typedef unsigned long       uintptr_t;
  #elif (ULLONG_MAX == 18446744073709551615ULL)
    typedef long long           int64_t;
    typedef unsigned long long  uint64_t;
    #if (sizeof(void*) == sizeof(unsigned long long))
      typedef unsigned long long uintptr_t;
    #else
      typedef unsigned int      uintptr_t;
    #endif
  #else
    #error "No 64-bit integer type available on this target"
  #endif
#endif /* size detection */

#endif /* UINT32_MAX */

/* Compile-time checks */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");
_Static_assert(sizeof(uintptr_t) == sizeof(void*), "uintptr_t must match pointer size");
#else
typedef char c_assert1[(sizeof(uint32_t) == 4)  ? 1 : -1];
typedef char c_assert2[(sizeof(uint64_t) == 8)  ? 1 : -1];
typedef char c_assert3[(sizeof(uintptr_t) == sizeof(void*)) ? 1 : -1];
#endif

#endif /* MY_STDINT_H */