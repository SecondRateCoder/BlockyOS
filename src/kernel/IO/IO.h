#pragma once
#include "../src/kernel/public/kernpublic.h"

typedef struct IObuf{
	void *buffer;
}IObuf;

typedef struct IO_Poll{
	uint16_t port_offset;
	uint32_t value;
}IO_Poll;

typedef struct IO_device{
	uint32_t bus,
			 slot,
			 function;
	uint16_t base_port;
}IO_device;

typedef struct true_IO{
	IObuf buffer;
	size_t buff_size;
	IO_device device;
}true_IO;

void _cdecl outl(uint16_t port, uint32_t val);
uint32_t __cdecl inl(uint16_t port);