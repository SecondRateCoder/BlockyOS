//The Program ID will be absolute zero.
#define KERNEL_ID (uint8_t[IDSize]){0}

void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);