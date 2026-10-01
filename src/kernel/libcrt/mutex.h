#pragma once

#include "kernel/libcrt/math/int.h"

#ifdef MATH_INT_H
typedef uint8_t						*CommonMutex;
#else
typedef unsigned char				*CommonMutex;
#endif
#define InvalidMutex                NULL
#define MutexDisableFlag			0x01
#define MutexEnableFlag				0x00
#define LockMutex(CM)				if(CM){*(CM) = MutexDisableFlag;}
#define UnlockMutex(CM)				if(CM){*(CM) = MutexEnableFlag;}
#define MutexPoll(Mtx)				if(Mtx){while(__check((volatile __typeof(*Mtx))(*Mtx), MutexDisableFlag)){;}}	LockMutex(Mtx);
#define InitMutex(Mtx)				static uint8_t  RAW__##Mtx;     CommonMutex Mtx = &RAW__##Mtx;					LockMutex(Mtx);
#define InitStaticMutex(Mtx)		CommonMutex Mtx = mcalloc(1, sizeof(uint8_t));									LockMutex(Mtx);
#define FreeStaticMutex(Mtx)		MutexPoll(Mtx)																	mfree(Mtx);
#define FreeMutex(Mtx)				MutexPoll(Mtx)