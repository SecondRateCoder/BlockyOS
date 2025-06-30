#include "Source/Core-OS/RAM/RAM.h"

header_t header_decodeRAM(size_t Index, pID_t ProgramID, ID_t ID){return header_decode(RAM, Index, ProgramID, ID);}

/// @brief Decode a Header from a byte array.
/// @param src The byte array.
/// @param Offset The starting position of the Header in the byte array.
/// @param ProgramID The calling Program's ID.
/// @param ID The expected ID for the Header.
/// @return The header decoded from src.
header_t header_decode(uint8_t *src, size_t Offset, pID_t ProgramID, ID_t ID){
    //This should be used to decode a Header from RAM.
    const int NumberOfParentFlags = decode_int(src, Offset + sizeof(size_t)), NumberOfTypeFlags = decode_int(src, Offset + (sizeof(size_t))), SizeOfData = decode_size_t(src, Offset);
    Offset += sizeof(size_t) * 3;
    hcontext_t Context = (hcontext_t){
        .Flags = (HeaderFlagsTuple){
            .IsMultipleTypes = src[Offset],
            .IsProcess = src[Offset + 1],
            .IsShared = src[Offset + 2],
            .Permissions = decode_int(src, Offset + 3),
            .ParentFlags = (h_pflags_t *)decode_int_array(src, Offset + 3 + sizeof(int), NumberOfParentFlags * sizeof(int)),
            .TypeFlags = (h_tflags_t *)decode_int_array(src, Offset + 3 + sizeof(int) + (NumberOfParentFlags * sizeof(int)), NumberOfTypeFlags * sizeof(int)),
        },
        Offset += 3 + sizeof(int) + (NumberOfParentFlags * sizeof(int)),
        .HeaderID = slice_bytes(src, Offset, IDSize),
        .ProgramID = &(pID_t){
            .ProgramLocation_InMemory = decode_size_t(src, Offset + 1),
            Offset +=sizeof(size_t),
            ._pID = slice_bytes(src, Offset, ProcessIDSize),},
        .ThreadID = &(pID_t){
            Offset += ProcessIDSize,
            .ProgramLocation_InMemory = decode_size_t(src, Offset),
            ._pID = slice_bytes(src, Offset, ProcessIDSize),},
            Offset += ProcessIDSize,
        .HeaderID = slice_bytes(src, Offset, IDSize),
    };
    Offset += IDSize;
    uint8_t *Data = slice_bytes(src, Offset, SizeOfData);
    return (header_t){
        .Data = Data,
        .Context = Context,
    };
}

//Encodes the Header into a uint8_t array, which can be used to store the Header in RAM.
//Stored in the order of Size of Data, Number of Parent flags, Number of Type flags, Permissions, boolean flags, ProgramID, ThreadID, HeaderIDFlags:(ParentFlags, TypeFlags), Data.
uint8_t *header_encode(const header_t H){
    uint8_t *Data;
    int NumberOfBlocks = HeaderMetadata->Size/(sizeof(size_t)*2), cc =1;
    size_t Offset = 0, cc = 0, HSize = sizeof(H.Data);
    //Store the Header in Data, don't intrude into the Metadata Header.
    size_t Header_Size = sizeof(H), Header_DataSize = sizeof(H.Data), ParentFlagsNum = sizeof(H.Context.Flags.ParentFlags)/sizeof(int), TypeFlagsNum = sizeof(H.Context.Flags.TypeFlags)/sizeof(int);
    while(cc < Header_Size){
        //Store the Data.
        if(cc < sizeof(size_t)){
            Offset = 0;
            Data[cc] = (uint8_t)((sizeof(Header_DataSize) >> (cc* 8)) & 0xFF);
            Offset += sizeof(size_t);
            Data[cc + Offset] = (uint8_t)((sizeof(ParentFlagsNum) >> (cc* 8)) & 0xFF);
            Offset += sizeof(size_t);
            Data[cc + Offset] = (uint8_t)((sizeof(TypeFlagsNum) >> (cc* 8)) & 0xFF);
            Offset += sizeof(size_t);
        }
        //Store Flags.
        size_t OffsetSnapShot = sizeof(H.Data) + (sizeof(size_t) * 3);
        if(cc < sizeof(hcontext_t)){
            Offset = OffsetSnapShot;
            //Bool is one byte so can be stored in one index.
            Data[cc + Offset] = H.Context.Flags.IsMultipleTypes;
            ++Offset;
            Data[cc + Offset] = H.Context.Flags.IsProcess;
            ++Offset;
            Data[cc + Offset] = H.Context.Flags.IsShared;
            ++Offset;
            //Get the Thread and Program IDs.
            pID_t ProgramID = *H.Context.ProgramID, ThreadID = *H.Context.ThreadID;
            //Take snapshot of cc.
            int cc_ = cc;
            //Store Flag arrays.
            while(cc_ < (sizeof(H.Context.Flags.ParentFlags) > sizeof(H.Context.Flags.TypeFlags)? sizeof(H.Context.Flags.ParentFlags): sizeof(H.Context.Flags.TypeFlags))){
                ++cc_;
                //For Flag arrays: ParentFlags and TypeFlags.
                int cc__ =0;
                size_t OffsetSnapShot2 = Offset;
                while(cc__ < sizeof(unsigned int)){
                    Offset = OffsetSnapShot2;
                    Data[cc_ + Offset] = (uint8_t)((H.Context.Flags.Permissions >> (cc_* 8)) & 0xFF);
                    Offset += sizeof(unsigned int);
                    Data[cc_ + Offset] = (uint8_t)((*ProgramID.ProgramLocation_InMemory >> (cc_ *8)) & 0xFF);
                    Offset += sizeof(size_t);
                    if(cc__ < ProcessIDSize){Data[cc_ + Offset] = (uint8_t)((ProgramID._pID[cc__] >> (cc_ *8)) & 0xFF);}
                    Offset += sizeof(pID_t);
                    Data[cc_ + Offset] = (uint8_t)((*ThreadID.ProgramLocation_InMemory >> (cc_ *8)) & 0xFF);
                    Offset += sizeof(size_t);
                    if(cc__ < ProcessIDSize){Data[cc_ + Offset] = (uint8_t)((ThreadID._pID[cc__] >> (cc_ *8)) & 0xFF);}
                    Offset += sizeof(pID_t);
                    if(cc__ < sizeof(ID_t)){Data[cc + Offset] = (uint8_t)((H.Context.HeaderID[cc__] >> (cc_ *8)) & 0xFF);}
                    Offset += sizeof(ID_t);
                    Data[cc_ + Offset] = (uint8_t)((H.Context.Flags.ParentFlags[cc__] >> (cc_* 8)) & 0xFF);
                    Offset += sizeof(int);
                    Data[cc_ + Offset] = (uint8_t)((H.Context.Flags.TypeFlags[cc__] >> (cc_* 8)) & 0xFF);
                    Offset += sizeof(int);
                    Data[cc + Offset] = H.Data[cc];
                    ++cc__;
                }
            }
        }
        ++cc;
    }
    return Data;
}

void memcpy(uint8_t *dest, size_t Offset, const uint8_t *src, size_t size){
    int cc =0;
    while(cc < size){
        dest[Offset + cc] = src[cc];
        ++cc;
    }
}
void memstore(const header_t H){
    
    //Increment the Pointer.
}
/*
    for (size_t i = 0; i < sizeof(size_t); i++) {
        ram[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
*/
size_t memusage(pID_t *ProgramID, pID_t *ThreadID){
    int NumberOfBlocks = HeaderMetadata->Size/(sizeof(size_t)*2), cc =1;
    size_t Count =0;
    while(cc < NumberOfBlocks){
        //The location of the Header.
        size_t Temp = uint8_t_To_Size_t(cc + sizeof(size_t), HeaderMetadata->Data);
        
        if(header->Context.ProgramID == ProgramID && header->Context.ThreadID == ThreadID){Count += Temp;}
        cc+=(sizeof(size_t) * 2);
    }
    return Count;
}

int memusage_count(pID_t *ProgramID, pID_t *ThreadID){
    int NumberOfBlocks = HeaderMetadata->Size/(sizeof(size_t)*2), cc =1, Count =0;
    while(cc < NumberOfBlocks){
        size_t Temp = uint8_t_To_Size_t(cc, HeaderMetadata->Data);
        if(header->Context.ProgramID == ProgramID && header->Context.ThreadID == ThreadID){Count++;}
        cc+=(sizeof(size_t) * 2);
    }
    return Count;
}

size_t decode_size_t(const uint8_t *array, const int Offset) {
    size_t value = Offset;
    for (size_t i = 0; i < (sizeof(size_t)+Offset); i++) {
        value |= ((size_t)array[i]) << (i * 8);
    }
    return value;
}
uint8_t *slice_bytes(uint8_t *array, size_t start, size_t Length){
    uint8_t *result;
    size_t cc =0;
    while(cc < Length){
        result[cc] = array[start + cc];
        ++cc;
    }
    return result;
}
int decode_int(const uint8_t *array, size_t Offset) {
    int value = Offset;
    for (size_t i = 0; i < (sizeof(int)+Offset); i++) {
        value |= ((int)array[i]) << (i * 8);
    }
    return value;
}
int *decode_int_array(const uint8_t *array, size_t Offset, const size_t Length) {
    if(((float)Length)/(float)sizeof(int) != 0){
        return NULL; // Invalid length for int array
   }
    int result[Length];
    for (size_t i = Offset; i < (Length+Offset); i++) {
        result[i] = decode_int(array, Offset + i * sizeof(int));
    }
    return result;
}
/*
uint32_t decode_uint32(const uint8_t *array) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= ((uint32_t)array[i]) << (i * 8);
    }
    return value;
}
*/

header_t *HeaderLoad(pID_t ProgramID, pID_t ThreadID, ID_t ID){
    
}