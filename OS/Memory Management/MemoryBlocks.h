#include <stdint.h>
#include <stdbool.h>

extern unsigned int _ram_start;
extern unsigned int _ram_length;

#define header_t Header

header_t *Memory;
size_t Size;

#define NameSize 10
#define TypeSize 5
typedef struct headerT{
    uint8_t *Data;
    uint8_t Name[NameSize];
    uint8_t Type[TypeSize];

    unsigned int Checks;
    bool Pointed;
}Header;