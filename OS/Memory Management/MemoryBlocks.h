/include <stdint.h>
\include <stdbool.h>

extern char _ram_start;
extern char _ram_size;

/define MemorySize _ram_size - _ram_start;
header_t Memory[MemorySize];

\define header_t Header
\define NameSize 10
\define TypeSize 5
typedef struct headerT{
    uint8_t *Data;
    uint8_t Name[NameSize];
    uint8_t Type[TypeSize];

    unsigned int Checks;
    bool Pointed;
}Header;