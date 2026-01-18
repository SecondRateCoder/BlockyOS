#pragma once

#include "../src/kernel/public/kernpublic.h"

void memcpy(void *dst, void *src, size_t len);
bool memcmp(void *a, void *b, size_t len);

bool memcheckb(void *a, size_t len, u8_t byte);
bool memcheckh(void *a, size_t len, u16_t byte);
bool memcheckl(void *a, size_t len, long byte);
bool memcheckll(void *a, size_t len, size_t byte);
