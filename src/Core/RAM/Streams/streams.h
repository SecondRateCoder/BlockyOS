#include "./src/Core/RAM/memtypes.h"

/*
[0] The Device index that the Stream sources it's data from,
[1] Flags: ([0]: read?, [1]: write?, [2]: unsafe?, [3]: protect?, [4]: kernel only?)
[2] The number of Bytes used to Index the Stream(X).
[3] -> [X] The Stream's index.
[X] -> [X + sizeof(size_t)] The Path to the Stream's origin path.
[(new)X] -> [(new)X + sizeof(size_t)] The end point of the Stream' origin path(in Memory.).
[(new)X] -> [(new)X+sizeof(size_t)] The size of a Stream's "package".
*/
#define STREAM_FLAG_READ_ACCESS    (1 << 0) // Bit 0 (00000001_2)
#define STREAM_FLAG_WRITE_ACCESS   (1 << 1) // Bit 1 (00000010_2)
#define STREAM_FLAG_UNSAFE         (1 << 2) // Bit 2 (00000100_2)
#define STREAM_FLAG_PROTECT        (1 << 3) // Bit 3 (00001000_2)
#define STREAM_FLAG_KERNEL_ONLY    (1 << 4) // Bit 4 (00010000_2)

#define stream_t STREAM
#define streamconfig_t stream_type
typedef void(* stream_inc)(stream_t*, void*);
typedef void(* stream_dec)(stream_t*, void*);
typedef size_t(* stream_read)(stream_t*, void*);

typedef struct STREAM{
    void *addr;
    size_t size;
    const stream_inc str_inc;
    const stream_dec str_dec;
    const stream_read str_read;
}STREAM;

typedef struct STREAM_FLAGS{uint8_t ParentProc[IDSize]; streamconfig_t strconfig;}STREAM_FLAGS;


typedef enum stream_type{
    STREAMTYPE_INPUT =0x21,
    STREAMTYPE_OUTPUT = 0x22,
    STREAMTYPE_VIDEO = 0x23,
    STREAMTYPE_AUDIO = 0x24,
    STREAMTYPE_FILE =0x25,
}stream_type;