#include "Source/Core-OS/RAM.h"

void HeaderStore(const header_t H){
    int NumberOfBlocks = HeaderMetadata->Size/(sizeof(size_t)*2), cc =1;
    size_t Temp = *Pointer;
    Pointer += sizeof(H.Data);
    //Add data about the Header.
    size_t HSize = sizeof(H.Data);
    while(cc < sizeof(size_t) * 2){
        if(cc > sizeof(size_t)){
            //Store the Header's Location in RAM.
            HeaderMetadata->Data[(NumberOfBlocks * sizeof(size_t)) + cc] = (uint8_t)((Temp >> (cc*8)) & 0xFF);
        }else{
            //Store the Header's size on RAM.
            HeaderMetadata->Data[(NumberOfBlocks * sizeof(size_t)) + cc] = (uint8_t)((HSize >> (cc*8)) & 0xFF);
        }
        ++cc;
    }
    //Store the Header in RAM.
    while(Pointer < HeaderMetadata->Adress){
        int cc = 1;
        size_t Header_Size = sizeof(H);
        while(cc < Header_Size){
            //Store the Data.
            if(cc < sizeof(H.Data)){
                RAM[Temp + cc] = H.Data[cc];
            }
            //Store Flags.
            if(cc < sizeof(hcontext_t)){
                RAM[Temp + cc + sizeof(H.Data)] = H.Context.Flags.IsMultipleTypes;
                ++cc;
                RAM[Temp + cc + sizeof(H.Data)] = H.Context.Flags.IsProcess;
                ++cc;
                RAM[Temp + cc + sizeof(H.Data)] = H.Context.Flags.IsShared;
                ++cc;
                int cc_ =cc, Offset =0;
                pID_t ProgramID = *H.Context.ProgramID, ThreadID = *H.Context.ThreadID;
                //Store Flag arrays.
                while(cc_ < (sizeof(H.Context.Flags.ParentFlags) > sizeof(H.Context.Flags.TypeFlags)? sizeof(H.Context.Flags.ParentFlags): sizeof(H.Context.Flags.TypeFlags))){
                    ++cc_;
                    if(cc_ < sizeof(H.Context.Flags.ParentFlags)){
                        int cc__ =0;
                        while(cc__ < sizeof(unsigned int)){
                            Offset += sizeof(H.Data);
                            RAM[Temp + cc_ + Offset] = (uint8_t)((H.Context.Flags.ParentFlags[cc__] >> (cc_* 8)) & 0xFF);
                            Offset +=sizeof(int);
                            RAM[Temp + cc_ + Offset] = (uint8_t)((H.Context.Flags.Permissions >> (cc_* 8)) & 0xFF);
                            Offset += sizeof(int);
                            RAM[Temp + cc_ + Offset] = (uint8_t)((H.Context.Flags.TypeFlags[cc__] >> (cc_* 8)) & 0xFF);
                            Offset += sizeof(int);
                            RAM[Temp + cc_ + Offset] = (uint8_t)((*ProgramID.ProgramLocation_InMemory >> (cc_ *8)) & 0xFF);
                            Offset += sizeof(size_t);
                            if(cc__ < ProgramIDSize){
                                RAM[Temp + cc_ + Offset] = (uint8_t)((ProgramID._pID[cc__] >> (cc_ *8)) & 0xFF);
                            }
                            Offset += sizeof(int);
                            RAM[Temp + cc_ + Offset] = (uint8_t)((*ThreadID.ProgramLocation_InMemory >> (cc_ *8)) & 0xFF);
                            Offset += sizeof(size_t);
                            if(cc__ < ProgramIDSize){
                                RAM[Temp + cc_ + Offset] = (uint8_t)((ThreadID._pID[cc__] >> (cc_ *8)) & 0xFF);
                            }
                            ++cc__;
                        }
                    }
                }
            }
            ++cc;
        }
        ++Pointer;
    }
    //Increment the Pointer.
}
/*
    for (size_t i = 0; i < sizeof(size_t); i++) {
        ram[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
*/

int HeadersUnderProcess(pID_t *ProgramID, pID_t *ThreadID){
    int NumberOfBlocks = HeaderMetadata->Size/(sizeof(size_t)*2), cc =1, Count =0;
    while(cc < NumberOfBlocks){
        size_t Temp[2] = {
            [0] = uint8_t_To_Size_t(cc, HeaderMetadata->Data),
            [1] = uint8_t_To_Size_t(cc + sizeof(size_t), HeaderMetadata->Data)};
        header_t *header = (header_t *)&RAM[Temp[0]];
        if(header->Context.ProgramID == ProgramID && header->Context.ThreadID == ThreadID){Count++;}
        cc+=(sizeof(size_t) * 2);
    }
}

size_t uint8_t_To_Size_t(const int Startfrom, const uint8_t *array) {
    size_t value = Startfrom;
    for (size_t i = 0; i < sizeof(size_t); i++) {
        value |= ((size_t)array[i]) << (i * 8);
    }
    return value;
}

/*
uint32_t decode_uint32(const uint8_t *array) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= ((uint32_t)array[i]) << (i * 8);
    }
    return value;
}
int decode_int(const uint8_t *array) {
    int value = 0;
    for (size_t i = 0; i < sizeof(int); i++) {
        value |= ((int)array[i]) << (i * 8);
    }
    return value;
}
*/