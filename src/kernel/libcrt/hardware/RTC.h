#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/hardware/IO/IO.h"

uint8_t CMOSRead(uint8_t reg);

static int CMOSUpdateInProgress(void);

typedef struct{
	uint8_t  second;
	uint8_t  minute;
	uint8_t  hour;
	uint8_t  day;
	uint8_t  month;
	uint32_t year;
}CMOS_time_t;

LibAPI uint32_t CMOSGetElapsedDays(uint8_t Year, uint8_t Month, uint8_t Days);
LibAPI void CMOSGetTime(CMOS_time_t *time);