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
typedef uint8_t *STREAM;

#define streamconfig_t stream_type
typedef struct stream_type{
    Input = 0x21,
    Output = 0x22,
    Video = 0x23,
    Audio = 0x24,
    File =0x25,
}stream_type;