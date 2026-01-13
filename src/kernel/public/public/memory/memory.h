#pragma once

#include "../src/kernel/public/kernpublic.h"

void memcpy(void *dst, void *src, size_t len);
bool memcmp(void *a, void *b, size_t len);