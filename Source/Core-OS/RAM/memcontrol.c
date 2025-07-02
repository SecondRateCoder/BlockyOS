void memcpy_unsafe(uint8_t *dest, const size_t offset, const uint8_t *src, const size_t n){
    size_t i =0;
    while(i < n){
        dest[offset +i] = src[i];
        i++;
    }
}