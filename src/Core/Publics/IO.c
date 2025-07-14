void outl(uint16_t port, uint32_t value){
    asm(
        "outl %0, %1":
        :
        "a"(value), "Nd"(port)
    );
    // __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t inl(uint16_t port){
    uint32_t ret;
    asm("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}