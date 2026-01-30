#pragma once
#include "../src/kernel/lib32/public/public/public.h"

#define RAMH_TYPE uint32_t
#include "./kernel/lib32/public/public/math/int/_bool.h"
#include "./kernel/lib32/public/public/math/int/_int.h"
#include "./kernel/lib32/public/public/memory/memory.h"

// stdfile
#include "./kernel/lib32/stdfile/stdfile.h"
#include "./kernel/lib32/stdfile/f-rat.h"
// stdio
#include "./kernel/lib32/stdio/stdio.h"
// stdkernel
#include "./kernel/lib32/stdkernel/stdkernel.h"
// stdlib
// #include "./kernel/lib32/stdlib"

//stdprogram
#include "./kernel/lib32/stdprogram/stdprogram.h"

#define aliasLen 10
typedef struct EnviromentVar{
    char alias[aliasLen];
    char value[PATHMAX];
}EnviromentVar;