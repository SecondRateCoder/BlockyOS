#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#define header_t Header
typedef struct Header{
    size_t Size;
    
}Header;

#define ProgramIDSize 10
#define pID_t _pID
typedef uint8_t _pID[ProgramIDSize];
#define IDSize 7
#define ID_t _ID
typedef uint8_t _ID[IDSize];

#define hcontext_t HeaderContext
typedef struct HContext{
    //The ID for the Program that the Header is used by.
    pID_t *ProgramID;

    pID_t ThreadID;
    ID_t HeaderID;
    
}HeaderContext;

#define h_pflags_t HeaderParentFlags
#define h_tflags_t HeaderTypeFlags

typedef struct HeaderFlagsTuple{
    h_pflags_t *ParentFlags;
    bool IsMultipleParents, IsMultipleTypes;
    h_tflags_t *TypeFlags;
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