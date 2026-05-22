#pragma once

// #include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/drivers/crypto/blake2/ref/blake2.h"

#define FUNCAPI __attribute__((ms_abi))

#define SAFEOP(A, B, CompAOp, CompA, CompBOp, CompB, Comp, OP, Alt)    (((A CompAOp CompA) Comp (B CompBOp CompB))? (A OP B): Alt)

#define safediv__(A, B)     SAFEOP((A), (B), ||, TRUE, !=, 0, &&, /, 1)

#define GUIDPRINT16 L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define GUIDPRINT "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
CHAR16 *_GUIDtoSTR(EFI_GUID guid);
void prGUID(EFI_GUID guid);

#define ARRSIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

#define cword_t CHAR16
#define GPTeNAMESIZE 72
#define GPTeNAMELEN (GPTeNAMESIZE / sizeof(cword_t))
typedef cword_t GPTeNSTR[GPTeNAMELEN];

#define __min(a, b) ((a) > (b)? (b): (a))
#define __max(a, b) ((a) < (b)? (b): (a))
#define abs(n) (((INTN)(n)) < 0? -(n): (n))

#define PATHSEP '/'
#define PATHnoSEP '\\'
#define CMDiMAX 256

#define enumdef(name, type)     typedef type name;  enum
#define bufdef(name, ptr, scale)		typedef struct name{ptr *name;		scale len;}name;

#define flagcheck(v, f) ((v & f) == f)
#define flagset(v, f)   (v |= f)
#define flagunset(v, f) (v &= ~f)

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static inline VOID IoWrite8Inline(UINT16 Port, UINT8 Value){
    __asm__ __volatile__ (
        "outb %0, %1"
        :
        : "a"(Value), "Nd"(Port)
    );}
#ifdef __DEBUG__
extern const EFI_PHYSICAL_ADDRESS DebugPort;
#define BUFDEFPRINT(mem, nbytes, countername)	for(UINTN countername = 0; countername < abs(nbytes); ++countername){IoWrite8Inline(DebugPort, ((uint8_t *)mem)[countername]);}
#else
#define BUFDEFPRINT(...)
#endif
typedef struct __efiDevNode{
	CHAR16 *nodeName;
	// This Node.
	EFI_DEVICE_PATH *local;
	// Protocol-Specific Data
	UINT8 protocolData[32];
	struct local__{
		// A ptr to the Parent Node, NULL if is the Parent.
		struct __efiDevNode *parent;
		// The #N of Children.
		UINT32 nChildren;
		// The Children of this Node.
		struct __efiDevNode **children;
	}local__;
}__efiDevNode;

typedef struct {
    UINT32 Attributes;
    UINT16 FilePathListLength;
    // CHAR16 Description[];          // variable length
    // EFI_DEVICE_PATH_PROTOCOL[];   // variable length
    // OptionalData[];               // variable length
} MY_LOAD_OPTION;

#define __efiIsFinal(node)      ((node)->local__.children == NULL)
#define __efiIsFirst(node)      ((node)->local__.parent == NULL)
#define __efiStepDown(node)     (!__efiIsFinal(node)? *((node)->local__.children): NULL)
#define __efiStepUp(node)     (!__efiIsFirst(node)? *((node)->local__.parent): NULL)
#define __efiStepRight(node, memory) (node = (!__efiIsFirst(node)? *((node)->local__.parent)->local__.parent->local__.children[memory++]))
#define __efiStepLeft(node, memory) (node = (!__efiIsFirst(node)? *((node)->local__.parent)->local__.parent->local__.children[memory--]))

__efiDevNode **loadDNodes(UINTN *nNodes);
EFI_DEVICE_PATH *getDevPath(EFI_DEVICE_PATH *dPath, UINT32 dType, UINT32 sType);
void DebugDevicePath(EFI_DEVICE_PATH *ROOT);
CHAR16 *DescribeDeviceNode(EFI_DEVICE_PATH *Node);
__efiDevNode *BuildDeviceTree(EFI_DEVICE_PATH *Path);
static CHAR16 *EfiMemoryTypeToStr(UINT32 type);
EFI_MEMORY_DESCRIPTOR *GetMemoryMap(UINTN *mapSize, UINTN *mapKey, UINTN *descSize, UINT32 *descVersion);

enumdef(strtokflags, uint32_t){
	strtok__ForceSameBorderingDelims = 0x1,
	strtok__ForceDifferentBorderingDelims,
	strtok__ForceStartingDelim = 0x80000000,
	strtok__ForceEndingDelim = 0x00002000
};

typedef struct strtok_t{
	/// @brief A duplicate of the Token.
	char *dup;
	/// @brief The recovered Token.
	char *tok;
	/// @brief The configured delims;
	char *delims;
	// The encountered delims
	char sdelim, edelim;
	UINT32 flags;
}strtok_t;

BOOLEAN isascii(char c);
BOOLEAN isdigit(char c);

BOOLEAN strcheck(char *s, char c);
INT64 strchecki(char *s, char c);
void *__memdup(void *mem, UINTN s);
UINT64 __getfcode(char *s_);
char *readbuf(UINTN s, CHAR16 *prefix);
EFI_STATUS getDriveMediaID(EFI_HANDLE Image, UINT32 *MediaID);
EFI_STATUS ValidateImageHandle(EFI_HANDLE Image);

strtok_t *strtok_i(char *in, char *delims, UINT32 enables);
char *strtok_k(strtok_t *tstate);
void strtok_d(strtok_t *tstate);

EFI_STATUS trng__(void *buffer, UINTN size);

char tolower(char upper);
char *str_tolower(char *s);

BOOLEAN __pattmatch(const char *pattern, const char *str);

UINT64 __strlen(char *s);
char *__strdup(char *s);
UINT64 __strspn(const char *s, const char *reject);

void __memset(void *dst, UINT8 val, UINT64 len);
void __safecopy(void *dst, void *src, UINT64 len);
void __memcpy(void * __restrict__ dst, void * __restrict__ src, UINT64 len);
UINT64 __memcmp(void * __restrict__ a, void * __restrict__ b, UINT64 len);

#ifdef __DEBUG__
#define __free(buffer)  \
	Print(L"\n[%s:%u]   Freeing Buffer    %p", (L"" __FILE__), __LINE__, buffer);   \
	FreePool(buffer);
#else
#define __free(buffer)  \
	FreePool(buffer);
#endif

#ifdef __DEBUG__
#define __calloc(nlen, nsize)		__calloc_(nlen, nsize);				Print(L"\n[%s:%u]", (L"" __FILE__), __LINE__);
#else
#define __calloc(nlen, nsize)		__calloc_(nlen, nsize);
#endif
void  *__calloc_(UINT64 nLen, UINT64 nSize);

#ifdef __DEBUG__
#define __realloc(mem, nlen, nsize) __realloc_(mem, nlen, nsize);	Print(L"    [%s:%u]", (L"" __FILE__), __LINE__);
#else
#define __realloc(mem, nlen, nsize)	__realloc_(mem, nlen, nsize);
#endif
void *__realloc_(void *memory, UINT64 currSize, UINT64 nSize);

BOOLEAN IsPartition(EFI_DEVICE_PATH *Dp);
EFI_DEVICE_PATH *GetDevicePath(EFI_HANDLE Handle);

GPTeNSTR *makeGPTeNSTR(char *str);

VOID RestartSystem();