#pragma once

#include "./kernel/lib32/stdmath/int/_int.h"


void memcpy(void *dst, void *src, size_t len);
bool memcmp(void *a, void *b, size_t len);
void memset(void *buffer, size_t len, uint8_t val);

bool memcheckb(void *a, size_t len, u8_t byte);
bool memcheckh(void *a, size_t len, u16_t byte);
bool memcheckl(void *a, size_t len, long byte);
bool memcheckll(void *a, size_t len, size_t byte);

bool memwithin(void *buffer, size_t size, void *address);