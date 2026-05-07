#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "efi.h"
#include "efilib.h"

#include "drivers/.disk/fs/frat.h"
#include "tools/tools.h"

#define BOOT_OPTION_ATTR (LOAD_OPTION_ACTIVE)