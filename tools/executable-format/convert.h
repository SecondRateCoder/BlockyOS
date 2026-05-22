#pragma once

#include "exec.h"
#include "pe+.h"

int convert_pe(const char *inpath, const char *outpath);
void dump_exec(const char *path);