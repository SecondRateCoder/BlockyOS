#include "Source/Core-OS/RAM.h"
#pragma region Alloca
void Alloca(){

}
/*
Entry point:
    Alloca(size_t Size),
Logic:
    Check if Pointer is pointing towards a used block:
        True: Point to a free block
    

Function: Point to a free block
    Get the size of the block
*/
#pragma endregion


#pragma region Helper functions

void AddMemBlock(const header_t H){
    int NumberOfBlocks = HeaderMetadata->Size/(sizeof(size_t)*2), cc =1;
    size_t Temp = *Pointer;
    Pointer += sizeof(H.Data);
    //Add data about the Header.
    size_t HSize = sizeof(H.Data);
    while(cc < sizeof(size_t) * 2){
        if(cc > sizeof(size_t)){
            //Store the Header's Location in RAM.
            HeaderMetadata->Data[(NumberOfBlocks * sizeof(size_t)) + cc] = (uint8_t)((Temp >> (cc*8)) && 0xFF);
        }else{
            //Store the Header's size on RAM.
            HeaderMetadata->Data[(NumberOfBlocks * sizeof(size_t)) + cc] = (uint8_t)((HSize >> (cc*8)) && 0xFF);
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
                int cc_ =cc;
                //Store Flag arrays.
                while(cc_ < (sizeof(H.Context.Flags.ParentFlags) > sizeof(H.Context.Flags.TypeFlags)? sizeof(H.Context.Flags.ParentFlags): sizeof(H.Context.Flags.TypeFlags))){
                    ++cc_;
                    if(cc_ < sizeof(H.Context.Flags.ParentFlags)){
                        int cc__ =0;
                        while(cc__ < sizeof(unsigned int)){
                            RAM[Temp + cc_ + sizeof(H.Data)] = ((H.Context.Flags.ParentFlags[cc__] >> (cc_* 8)) && 0xFF);
                            ++cc__;
                        }
                    }
                    if(cc_ < sizeof(H.Context.Flags.TypeFlags)){
                        int cc__ =0;
                        while(cc__ < sizeof(unsigned int)){
                            RAM[Temp + cc_ + sizeof(H.Data) + sizeof(int)] = ((H.Context.Flags.TypeFlags[cc__] >> (cc_* 8)) && 0xFF);
                            ++cc__;
                        }
                    }
                    if(cc_ < sizeof(int)){
                        RAM[Temp + cc_ + sizeof(H.Data) + (sizeof(int) *2)] = ((H.Context.Flags.Permissions >> (cc_* 8)) && 0xFF);
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
#pragma endregion


#pragma region De-alloca_Free

#pragma endregion