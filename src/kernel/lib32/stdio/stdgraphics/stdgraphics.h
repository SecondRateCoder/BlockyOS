#include "./kernel/lib32/stddevice/PCI/PCI.h"

typedef struct VBEModeBlock{
    uint16_t modeattributes;
    uint8_t WindowAattrributes,
            WindowBattributes;
    uint16_t WindowGranularityinKB;
}PACKEDSTRUCT VBEModeBlock;
// 00h    WORD    mode attributes (see #00080)
// 02h    BYTE    window attributes, window A (see #00081)
// 03h    BYTE    window attributes, window B (see #00081)
// 04h    WORD    window granularity in KB
// 06h    WORD    window size in KB
// 08h    WORD    start segment of window A (0000h if not supported)
// 0Ah    WORD    start segment of window B (0000h if not supported)
// 0Ch    DWORD   -> FAR window positioning function (equivalent to AX=4F05h)
// 10h    WORD    bytes per scan line
// ---remainder is optional for VESA modes in v1.0/1.1, needed for OEM modes---
// 12h    WORD    width in pixels (graphics) or characters (text)
// 14h    WORD    height in pixels (graphics) or characters (text)
// 16h    BYTE    width of character cell in pixels
// 17h    BYTE    height of character cell in pixels
// 18h    BYTE    number of memory planes
// 19h    BYTE    number of bits per pixel
// 1Ah    BYTE    number of banks
// 1Bh    BYTE    memory model type (see #00082)
// 1Ch    BYTE    size of bank in KB
// 1Dh    BYTE    number of image pages (less one) that will fit in video RAM
// 1Eh    BYTE    reserved (00h for VBE 1.0-2.0, 01h for VBE 3.0)
// ---VBE v1.2+ ---
// 1Fh    BYTE    red mask size
// 20h    BYTE    red field position
// 21h    BYTE    green mask size
// 22h    BYTE    green field size
// 23h    BYTE    blue mask size
// 24h    BYTE    blue field size
// 25h    BYTE    reserved mask size
// 26h    BYTE    reserved mask position
// 27h    BYTE    direct color mode info

// bit 0:
// Color ramp is programmable

// bit 1:
// Bytes in reserved field may be used by application
// ---VBE v2.0+ ---
// 28h    DWORD   physical address of linear video buffer
// 2Ch    DWORD   pointer to start of offscreen memory
// 30h    WORD    KB of offscreen memory
// ---VBE v3.0 ---
// 32h    WORD    bytes per scan line in linear modes
// 34h    BYTE    number of images (less one) for banked video modes
// 35h    BYTE    number of images (less one) for linear video modes

// 36h    BYTE    linear modes:
// Size of direct color red mask (in bits)

// 37h    BYTE    linear modes:
// Bit position of red mask LSB (e.g. shift count)

// 38h    BYTE    linear modes:
// Size of direct color green mask (in bits)

// 39h    BYTE    linear modes:
// Bit position of green mask LSB (e.g. shift count)

// 3Ah    BYTE    linear modes:
// Size of direct color blue mask (in bits)

// 3Bh    BYTE    linear modes:
// Bit position of blue mask LSB (e.g. shift count)

// 3Ch    BYTE    linear modes:
// Size of direct color reserved mask (in bits)

// 3Dh    BYTE    linear modes:
// Bit position of reserved mask LSB
// 3Eh    DWORD   maximum pixel clock for graphics video mode, in Hz
// 42h 190 BYTEs  reserved (0)