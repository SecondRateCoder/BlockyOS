struct MPR {
    uint32_t base;
    uint32_t size;
    uint8_t  permissions; // bits for R/W/X, user/supervisor
};