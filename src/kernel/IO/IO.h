#pragma once
#include "../public/kernpublic.h"

typedef struct _IO{const size_t ID;}_IO;
typedef struct IO_device{
	uint32_t bus,
			 slot,
			 func;
}IO_device;

typedef struct true_IO{
	void *buffer;
	size_t buff_size;
	size_t ID;
	IO_device device;
}true_IO;