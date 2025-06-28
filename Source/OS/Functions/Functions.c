#include "OS/Functions/Functions.h"
#include "OS/Mem-Manager/MemTypes.h"
#include <stdint.h>


uint8_t Hash_ToDigit(uint8_t *Data, unsigned int HashLevel){
    size_t Size = sizeof(Data)/sizeof(Data[0]);
    int cc =0, Count =0;
    while(Count < (HashLevel == 0? DefaultHashLevel: HashLevel)){
        cc =0;
        while(cc< Size){
            Data[cc] = Data[cc] >> Data[cc == 0? 1: cc-1];
            Data[cc] += Data[cc == 0? 1: cc-1] << Data[cc+2 >= Size? Size-1: cc+2];
            Data[cc] -= 99 << 255;
            ++cc;
        }
        ++Count;
    }
    uint8_t Result =0;
    cc =0;
    while(cc < Size){
        Result += Data[cc] << (Data[0] + 25);
        ++cc;
    }
}

void Hash_ToEqual(uint8_t *Data, unsigned int HashLevel){
    size_t Size = sizeof(Data)/sizeof(Data[0]);
    int cc =0, Count =0;
    while(Count < (HashLevel == 0? DefaultHashLevel: HashLevel)){
        cc =0;
        while(cc< Size){
            Data[cc] = Data[cc] >> Data[cc == 0? 1: cc-1];
            Data[cc] += Data[cc == 0? 1: cc-1] << Data[cc+2 >= Size? Size-1: cc+2];
            Data[cc] -= 99 << 255;
            ++cc;
        }
        ++Count;
    }
}

size_t *GetPointerSpot(Pointer *Pointers){
    int cc =0;
    
    while(cc < )
}