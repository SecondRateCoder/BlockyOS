#include <stdint.h>
#include <stdbool.h>

extern int _ram_start;
extern int _ram_end;
extern size_t _ram_length;
extern int heap_start;
extern int heap_end;


#define ConventionalStackSize 1048576//In bytes
#define header_t Header
#define NameSize 10
#define TypeSize 5
#define IDSize 10
#define ProgramIDSize 7
#define PageSize 1024 //1 Kilobyte.

header_t *Memory[_ram_length];

typedef void *Function(header_t *Header, void *args);

typedef enum WorkDirection{
    ForWard =0,
    BackWard =1,
    Omni-Directional =2,
    StartBeginning_GoForward =3
}WorkDirection;

typedef struct BlockContext{
    //The byte size of the whole object, this is to support in data retrieval.
    size_t WholeObjectSize;
    //For identifying what Program the struct can be bound to.
    uint8_t ProgramID[ProgramIDSize];
    uint8_t ID[IDSize]
}BlockContext;

typedef struct headerT{
    uint8_t Data[PageSize];
    uint8_t Name[NameSize];
    uint8_t Type[TypeSize];

    unsigned int Checks;
    bool Pointed;

    BlockContext tContext;
    header_t *NextBlock;
    header_t *LastBlock;
}Header;