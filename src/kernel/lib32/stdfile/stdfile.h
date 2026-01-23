#include "./kernel/public/public/math/int/_int.h"

typedef uint8_t sectorbuff[128];
size_t sectorpointer;
sectorbuff sectorhandles[10];
uint8_t writeptr;

size_t sectortell();
void sectorseek(ssize_t offset);
void *sectorread_(unsigned long bytes);
void sectorwrite(void *buffer, size_t buffsize);
void sectorread(void *buffer, size_t buffsize, size_t bytes);
