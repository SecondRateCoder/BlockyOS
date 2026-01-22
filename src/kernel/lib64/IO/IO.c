#include "../src/kernel/IO/IO.h"

size_t IO_Counter;
true_IO *Open_IO;

/// @brief Register a new true IO item
/// @param device The device the item should point to.
/// @param buffer_size The expected size of the buffer.
/// @return A pointer to the device.
true_IO *IO_Reg(IO_device device, size_t buffer_size){
    for(size_t cc =0; cc < IO_Counter; ++cc){
        if(device.bus == Open_IO[cc].device.bus &&
            device.slot == Open_IO[cc].device.slot &&
            device.function == Open_IO[cc].device.function
        ){return NULL;}
    }
    Open_IO[IO_Counter] = (true_IO){
        .buffer.buffer = alloca_weak(buffer_size),
        .buff_size = buffer_size,
        .device = device,
    };
    IO_Counter++;
    return (Open_IO + IO_Counter);
}

/// @brief Get an IO Wrapper for a true_IO type
/// @param ID The ID of the true_IO
/// @return A IOBuffer wrapper.
IObuf *get_buffer(size_t ID){
    if(ID < IO_Counter){return &Open_IO[ID].buffer;}
    return NULL;
}

/// @brief Push multiple uint32_t types intovarious ports, Check: @typedef IO_Poll
/// @param ID The ID of the device to be polled.
/// @param argp The argument, Check: @typedef IO_Poll
/// @param num_data The number of items in argp
/// @param outport_offset The port offset to be check for a return value.
/// @return The value from the out port offset.
uint32_t IO_Poll(size_t ID, uint16_t *(argp[3]), uint16_t num_data, uint16_t outport_offset){
    for(uint16_t cc = 0; cc < num_data; ++cc){
        uint16_t temp[3] = {argp[cc][0], argp[cc][1], argp[cc][2]};
        outl(Open_IO[ID].device.base_port + temp[0], ((uint32_t *)argp)[0]);
    }
    return inl(Open_IO[ID].device.base_port + outport_offset);
}