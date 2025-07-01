#include "Source/Core-OS/RAM/RAM.h"

uint8_t *header_peekunsafe(uint8_t *src, size_t Offset, hpeek_t PeekType){return decode_size_t(src, Offset + PeekType);}
uint8_t *header_peek(uint8_t *src, size_t Offset, pID_t ProgramID, hpeek_t PeekType){
    //This should be used to peek into a Header in RAM.
    pID_t ProgramID_ =decode_id(header_peekunsafe(src, Offset, HeaderAttrOffset_ProgramID), 0);
    //If address is invalid and the ProgramID does not match and HeaderID and ID do not match, return 0.
    if(!(address_validate(Offset) && 
        ProgramID_.ProgramLocation_InMemory == ProgramID.ProgramLocation_InMemory && compare_array(ProgramID_._pID, ProgramID._pID))){return NULL;}
        switch(PeekType){
            case HeaderAttrOffset_DataSize:
            case HeaderAttrOffset_NumberOfParentFlags:
            case HeaderAttrOffset_NumberOfTypeFlags:
            case HeaderAttrOffset_ProgramIDLocation:
            case HeaderAttrOffset_ThreadIDLocation:
                return slice_bytes(src, Offset + PeekType, sizeof(size_t));
            case HeaderAttrOffset_IsMultipleTypes:
            case HeaderAttrOffset_IsProcess:
            case HeaderAttrOffset_IsShared:
                return slice_bytes(src, Offset + PeekType, sizeof(bool));
            case HeaderAttrOffset_Permissions:
                return slice_bytes(src, Offset + PeekType, sizeof(int));
            case HeaderAttrOffset_ProgramID:
            case HeaderAttrOffset_ThreadID:
            case HeaderAttrOffset_HeaderID:
                return slice_bytes(src, Offset + PeekType, ProcessIDSize + sizeof(size_t));
            case HeaderAttrOffset_HeaderSize:
                return slice_bytes(src, Offset, 
                    decode_size_t(header_peek(src, Offset, ProgramID, HeaderAttrOffset_DataSize), 0) + 
                        (decode_size_t(header_peek(src, Offset, ProgramID, HeaderAttrOffset_NumberOfParentFlags), 0) +decode_size_t(header_peek(src, Offset, ProgramID, HeaderAttrOffset_NumberOfTypeFlags), 0))*sizeof(int)+
                        HeaderStaticSize);
            default:
                return slice_bytes(src, Offset + PeekType, ProcessIDSize + sizeof(size_t));
        }
}

/// @brief Decode a Header from a byte array.
/// @param src The byte array.
/// @param Offset The starting position of the Header in the byte array.
/// @param ProgramID The calling Program's ID.
/// @param ID The expected ID for the Header.
/// @return The header decoded from src.
header_t header_decode(uint8_t *src, size_t Offset, pID_t ProgramID, ID_t ID){
    if(!address_validate(Offset)){return (header_t){.Data = NULL, .Context = NULL};}else{
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
}

//Encodes the Header into a uint8_t array, which can be used to store the Header in RAM.
//Stored in the order of Size of Data, Number of Parent flags, Number of Type flags, Permissions, boolean flags, ProgramID, ThreadID, HeaderIDFlags:(ParentFlags, TypeFlags), Data.
uint8_t *header_encode(const header_t H, uint8_t ParentFlagsSize, uint8_t TypeFlagsSize, size_t DataSize){
    uint8_t *Data;
    encode_hcontext_t(Data, H.Context, 0);
    memappend_unsafe(Data, false, H.Data, sizeof(H.Data));
    // size_t cc =1;
    // size_t Offset = 0, cc = 0, HSize = sizeof(H.Data);
    // //Store the Header in Data, don't intrude into the Metadata Header.
    // size_t Header_Size = sizeof(H), Header_DataSize = sizeof(H.Data), ParentFlagsNum = sizeof(H.Context.Flags.ParentFlags)/sizeof(int), TypeFlagsNum = sizeof(H.Context.Flags.TypeFlags)/sizeof(int);
    // while(cc < Header_Size){
    //     //Store the Data.
    //     if(cc < sizeof(size_t)){
    //         Offset = 0;
    //         Data[cc] = (uint8_t)((sizeof(Header_DataSize) >> (cc* 8)) & 0xFF);
    //         Offset += sizeof(size_t);
    //         Data[cc + Offset] = (uint8_t)((sizeof(ParentFlagsNum) >> (cc* 8)) & 0xFF);
    //         Offset += sizeof(size_t);
    //         Data[cc + Offset] = (uint8_t)((sizeof(TypeFlagsNum) >> (cc* 8)) & 0xFF);
    //         Offset += sizeof(size_t);
    //     }
    //     //Store Flags.
    //     size_t OffsetSnapShot = sizeof(H.Data) + (sizeof(size_t) * 3);
    //     if(cc < sizeof(hcontext_t)){
    //         Offset = OffsetSnapShot;
    //         //Bool is one byte so can be stored in one index.
    //         Data[cc + Offset] = H.Context.Flags.IsMultipleTypes;
    //         ++Offset;
    //         Data[cc + Offset] = H.Context.Flags.IsProcess;
    //         ++Offset;
    //         Data[cc + Offset] = H.Context.Flags.IsShared;
    //         ++Offset;
    //         //Get the Thread and Program IDs.
    //         pID_t ProgramID = *H.Context.ProgramID, ThreadID = *H.Context.ThreadID;
    //         //Take snapshot of cc.
    //         int cc_ = cc;
    //         //Store Flag arrays.
    //         while(cc_ < (sizeof(H.Context.Flags.ParentFlags) > sizeof(H.Context.Flags.TypeFlags)? sizeof(H.Context.Flags.ParentFlags): sizeof(H.Context.Flags.TypeFlags))){
    //             ++cc_;
    //             //For Flag arrays: ParentFlags and TypeFlags.
    //             int cc__ =0;
    //             size_t OffsetSnapShot2 = Offset;
    //             while(cc__ < sizeof(unsigned int)){
    //                 Offset = OffsetSnapShot2;
    //                 Data[cc_ + Offset] = (uint8_t)((H.Context.Flags.Permissions >> (cc_* 8)) & 0xFF);
    //                 Offset += sizeof(unsigned int);
    //                 Data[cc_ + Offset] = (uint8_t)((*ProgramID.ProgramLocation_InMemory >> (cc_ *8)) & 0xFF);
    //                 Offset += sizeof(size_t);
    //                 if(cc__ < ProcessIDSize){Data[cc_ + Offset] = (uint8_t)((ProgramID._pID[cc__] >> (cc_ *8)) & 0xFF);}
    //                 Offset += sizeof(pID_t);
    //                 Data[cc_ + Offset] = (uint8_t)((*ThreadID.ProgramLocation_InMemory >> (cc_ *8)) & 0xFF);
    //                 Offset += sizeof(size_t);
    //                 if(cc__ < ProcessIDSize){Data[cc_ + Offset] = (uint8_t)((ThreadID._pID[cc__] >> (cc_ *8)) & 0xFF);}
    //                 Offset += sizeof(pID_t);
    //                 if(cc__ < sizeof(ID_t)){Data[cc + Offset] = (uint8_t)((H.Context.HeaderID[cc__] >> (cc_ *8)) & 0xFF);}
    //                 Offset += sizeof(ID_t);
    //                 Data[cc_ + Offset] = (uint8_t)((H.Context.Flags.ParentFlags[cc__] >> (cc_* 8)) & 0xFF);
    //                 Offset += sizeof(int);
    //                 Data[cc_ + Offset] = (uint8_t)((H.Context.Flags.TypeFlags[cc__] >> (cc_* 8)) & 0xFF);
    //                 Offset += sizeof(int);
    //                 Data[cc + Offset] = H.Data[cc];
    //                 ++cc__;
    //             }
    //         }
    //     }
    //     ++cc;
    // }
    return Data;
}

void memcpy_unsafe(uint8_t *dest, size_t Offset, const uint8_t *src, size_t size){
    int cc =0;
    while(cc < size){
        dest[Offset + cc] = src[cc];
        ++cc;
    }
}
void memappend_unsafe(uint8_t *dest, bool GrowsDownwards, const uint8_t *src, size_t size){
    //search for the end of the destination array.
    size_t cc =Offset, ArraySize = sizeof(dest)/sizeof(uint8_t);
    switch(GrowsDownwards){
        case true:
            memcpy_unsafe(dest, ArraySize, src, size);
            //If the array grows downwards, we need to find the end of the array.
            break;
        case false:
            memcpy_unsafe(dest, ArraySize-size, src, size);
            dest -= Offset;
            break;
    }
}
/*
    for (size_t i = 0; i < sizeof(size_t); i++) {
        ram[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
*/
size_t memusage(pID_t *ProgramID, pID_t *ThreadID){
    size_t Count =0, cc =1;
    while(cc < NumberOfBlocks){
        //The location of the Header.
        size_t Temp = decode_size_t(HeaderMetadata->Data, cc);
        if(header_peek(RAM, Temp, *ProgramID, 0) != NULL){Count += Temp;}
        cc+=(sizeof(size_t) * 2);
    }
    return Count;
}
int memusage_count(pID_t *ProgramID, pID_t *ThreadID){
    size_t cc =1, Count =0;
    while(cc < NumberOfBlocks){
        size_t Temp = decode_size_t(cc, HeaderMetadata->Data);
        if(header_peek(RAM, Temp, *ProgramID, 0) != NULL){++Count;}
        cc+=(sizeof(size_t) * 2);
    }
    return Count;
}

pID_t decode_id(uint8_t *array, size_t StartFrom){
    pID_t ID;
    ID.ProgramLocation_InMemory = decode_size_t(array, StartFrom);
    for (size_t i = sizeof(size_t); i < ProcessIDSize; i++){
        ID._pID[i] = (uint8_t)((array[i] >> (i * 8)) & 0xFF);
    }
    return ID;
}
size_t decode_size_t(const uint8_t *array, const int Offset){
    size_t value = Offset;
    for (size_t i = 0; i < (sizeof(size_t)+Offset); i++) {
        value |= ((size_t)array[i]) << (i * 8);
    }
    return value;
}
int decode_int(const uint8_t *array, size_t Offset){
    int value;
    for (size_t i = Offset; i < (sizeof(int)+Offset); i++) {
        value |= ((int)array[i]) << (i * 8);
    }
    return value;
}
int *decode_int_array(const uint8_t *array, size_t Offset, const size_t Length){
    if(((float)Length)/(float)sizeof(int) != 0){return NULL;}
    int result[Length];
    for (size_t i = Offset; i < (Length+Offset); i++) {
        result[i] = decode_int(array, Offset + i * sizeof(int));
    }
    return result;
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

size_t clamp_size_t(size_t lower, size_t upper, size_t value){return value > upper? upper : (value < lower? lower : value);}

void encode_size_t(uint8_t *array, size_t value, size_t offset){
    for (size_t i = 0; i < sizeof(size_t); i++) {
        array[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
}
void encode_int(uint8_t *array, int value, size_t offset){
    for (size_t i = 0; i < sizeof(int); i++) {
        array[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
}
void encode_hcontext_t(uint8_t *array, hcontext_t Context, size_t offset){
    //Encode the Header Context into the array.
    encode_size_t(array, Context.Flags.IsMultipleTypes, offset);
    encode_size_t(array, Context.Flags.IsProcess, offset + sizeof(bool));
    encode_size_t(array, Context.Flags.IsShared, offset + sizeof(bool) * 2);
    encode_int(array, Context.Flags.Permissions, offset + sizeof(bool) * 3);
    //Encode ParentFlags and TypeFlags.
    size_t ParentFlagsOffset = offset + sizeof(bool) * 3 + sizeof(int);
    for (size_t i = 0; i < sizeof(Context.Flags.ParentFlags)/sizeof(int); i++) {
        encode_int(array, Context.Flags.ParentFlags[i], ParentFlagsOffset + i * sizeof(int));
    }
    size_t TypeFlagsOffset = ParentFlagsOffset + (sizeof(Context.Flags.ParentFlags)/sizeof(int) * sizeof(int));
    for (size_t i = 0; i < sizeof(Context.Flags.TypeFlags)/sizeof(int); i++) {
        encode_int(array, Context.Flags.TypeFlags[i], TypeFlagsOffset + i * sizeof(int));
    }
    //Encode ProgramID and ThreadID.
    encode_size_t(array, Context.ProgramID->ProgramLocation_InMemory, TypeFlagsOffset + (sizeof(Context.Flags.TypeFlags)/sizeof(int) * sizeof(int)));
    memcpy_unsafe(array, TypeFlagsOffset + (sizeof(Context.Flags.TypeFlags)/sizeof(int) * sizeof(int)) + sizeof(size_t), Context.ProgramID->_pID, ProcessIDSize);
    encode_size_t(array, Context.ThreadID->ProgramLocation_InMemory, TypeFlagsOffset + (sizeof(Context.Flags.TypeFlags)/sizeof(int) * sizeof(int)) + ProcessIDSize);
    memcpy_unsafe(array, TypeFlagsOffset + (sizeof(Context.Flags.TypeFlags)/sizeof(int) * sizeof(int)) + ProcessIDSize + sizeof(size_t), Context.ThreadID->_pID, ProcessIDSize);
}

bool space_validate(size_t address, size_t concurrent_size){
    size_t cc =0;
    while(cc < NumberOfBlocks){
        size_t SizeOfBlock = decode_size_t(HeaderMetadata->Data, cc + sizeof(hcontext_t) + sizeof(size_t)), Address = decode_size_t(HeaderMetadata->Data, cc + sizeof(hcontext_t));
        if(clamp_size_t(Address, Address + Size, address + concurrent_size) != address + size){return false;}
        cc+=(MetadataBlockCount);
    }
    return false;
}
bool address_validate(size_t Offset){
    int cc =0;
    while(cc < NumberOfBlocks){
        if(decode_size_t(HeaderMetadata->Data, cc) == Offset){return true;}
    }
    return false;
}

bool compare_array(uint8_t *array1, uint8_t *array2){
    if(sizeof(array1) != sizeof(array2)){return false;}
    int cc =0;
    while(cc < sizeof(array1)){
        if(array1[cc] != array2[cc]){return false;}
        ++cc;
    }
    return true;
}

bool used_at(size_t address){
    if(!address_validate(address)){return false;}
    size_t address_[2] = used_bounds();
    return (address_[0] >= address && address <= address_[1]);
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