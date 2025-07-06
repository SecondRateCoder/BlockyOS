#include "./Source/Core-OS/RAM/memtypes.h"

#pragma region Alloca
void *Alloca(size_t Size, ID_t ProcessID){
    size_t freeaddr = address_pointfree(RAMMeta, Size);
}
#pragma endregion