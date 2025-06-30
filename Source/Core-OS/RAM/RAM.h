#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OSBits (sizeof(void*) * 8)
#define ProcessIDSize 10
#define IDSize 7
#define pID_t _pID
#define ID_t _ID
#define h_pflags_t HeaderParentFlags
#define h_tflags_t HeaderTypeFlags
#define hcontext_t HeaderContext
#define header_t Header
typedef struct _pID{uint8_t _pID[ProcessIDSize];    size_t *ProgramLocation_InMemory;}_pID;
typedef uint8_t _ID[IDSize];

//The start of RAM in Device Memory, with pre-defined space for the OS as well as thre length of RAM (with the OS taken into consideraion.).
extern uintptr_t _ram_start, _ram_length;
//A Pointer Table representation for RAM.
#define RAM RAM_
volatile uint8_t *RAM_ = (uint8_t *)&_ram_start;
//Points to free memory in RAM, grows upwards from RAM start.
volatile size_t *Pointer;
//A pointer stating how many memory blocks there are, grows downwards from the RAM end.
volatile header_t *HeaderMetadata;

typedef void (*HeaderFunction)(header_t *H, void *args);

typedef struct Header{
    uint8_t *Data;
    hcontext_t Context;
}Header;

typedef struct HContext{
    //The ID for the Program that the Header is used by && The ID for the thread using Header.
    pID_t *ProgramID, *ThreadID;
    ID_t HeaderID;
    HeaderFlagsTuple Flags;
}HeaderContext;

typedef struct HeaderFlagsTuple{
    //The Parent(s) that the Header has, since it can only have 3 values.
    h_pflags_t *ParentFlags;
    //The Type(s) and Arg(s) that the Header has, since it can only have 6 values.
    h_tflags_t *TypeFlags;
    //Is the Data shared by multiple Threads/Programs, does the Header have multiple Type(s) and/or Arg(s).
    bool IsShared, IsMultipleTypes, IsProcess;
    
    unsigned int Permissions;
}HeaderFlagsTuple;

typedef enum HeaderFlags{
    Kernel,
    ChildProcess,
    Thread,
}HeaderParentFlags;

typedef enum HeaderFlags{
    ProgramBinaries,
    Encrypted,
    ProgramProtected,
    KernelProtected,
    Un_Protected,
    Data,
}HeaderTypeFlags;

typedef enum HeaderAttrOffset{
    HeaderAttrOffset_DataSize = 0,
    HeaderAttrOffset_NumberOfParentFlags = sizeof(size_t),
    HeaderAttrOffset_NumberOfTypeFlags = sizeof(size_t) * 2,
    HeaderAttrOffset_Permissions = sizeof(size_t) * 3,
    HeaderAttrOffset_IsMultipleTypes = sizeof(size_t) * 3 + sizeof(int),
    HeaderAttrOffset_IsProcess = sizeof(size_t) * 3 + sizeof(int) + 1,
    HeaderAttrOffset_IsShared = sizeof(size_t) * 3 + sizeof(int) + 2,
    //The Flags and Data.
    HeaderAttrOffset_Pointers = sizeof(size_t) * 3 + sizeof(int) + 3,
}HeaderAttrOffset;