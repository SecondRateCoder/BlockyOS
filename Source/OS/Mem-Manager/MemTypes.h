#include <stdint.h>
#include <stdbool.h>

extern int _ram_start;
extern int _ram_end;
extern size_t _ram_length;
extern int heap_start;
extern int heap_end;
extern size_t *Pointer;

#define ConventionalStackSize 1048576//In bytes
#define header_t Header
#define NameIDSize 10
#define TypeIDSize 5
#define IDSize 7
#define ProgramIDSize 7
#define PageSize 1024 //1 Kilobyte.
#define ProtectedChecks 10000
#define Un_ProtectedChecks 100
#define RetryLimit 5

volatile header_t **Memory;

typedef void ( *Function)(header_t *, void *);

typedef struct Pointer{
    size_t Pointer;
    bool Used;
}Pointer;

typedef enum WorkDirection{
    //Move Forward.
    ForWard =0,
    //Move backward.
    BackWard =1,
    //Complete the function thing in a softer, lenient fashion.
    Omni_Directional =2,
    //For when the function route has reached the top-most Header in the MemType.
    StartBeginning_GoForward =3,
    //For when the function route has reached the deepest Header in the MemType.
    StartEnd_GoBackward =4,
    //Do function om single block.
    Break =5
}WorkDirection;

#define bcontext_t BlockContext
typedef struct BlockContext{
    //The byte size of the whole object, this is to support in data retrieval.
    size_t WholeObjectSize;
    //For identifying what Program the struct can be bound to.
    uint8_t ProgramID[ProgramIDSize];
    uint8_t ID[IDSize];
}BlockContext;

typedef struct headerT{
    uint8_t Data[PageSize];
    uint8_t Name[NameIDSize];
    uint8_t Type[TypeIDSize];

    unsigned int Checks;
    bool Pointed;

    BlockContext tContext;
    header_t *NextBlock;
    header_t *LastBlock;
}Header;

void DoOnAllBlocks(Function *f, void *args, header_t *StartFrom, WorkDirection Direction);
void PointAt(header_t Header, WorkDirection dir, size_t *Num);
bool Point_PointerSimilar(header_t RefTo, size_t *Pointer_);
uint8_t *AssignID(uint8_t ProgramID[ProgramIDSize]);
int HeaderCount(uint8_t ProgramID[ProgramIDSize]);
bool Point_PointerFree(size_t *Pointer_);
bool IsPointingFree(size_t Pointer_);
bool IsPointingUsed(size_t Pointer_);
int MemorySize();