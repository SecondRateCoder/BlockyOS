#include "OS/Mem-Manager/MemTypes.h"
#include "OS/Functions/Functions.h"

void Free_(header_t Header, void *args){
    Header = (header_t){
        .Data = (uint8_t[PageSize]){},
        .Name = (uint8_t[NameIDSize]){},
        .Type = (uint8_t[TypeIDSize]){},
        .Checks = -1,
        .Pointed = false,
        .tContext = (bcontext_t){
            .WholeObjectSize = 0,
            .ProgramID = (uint8_t[ProgramIDSize]){},
            .ID = (uint8_t[IDSize]){},
        },
        .NextBlock = NULL,
        .LastBlock = NULL
    };
}

void Free(header_t *Header){
    int cc =0;
    while(cc < MemorySize()){
        if(Memory[cc] == Header){break;}
        ++cc;
    }
    if(Header->LastBlock != NULL || Header->NextBlock != NULL){
        DoOnAllBlocks(&Free_, (void *){0b00000000}, Header, Omni_Directional);
    }else{
        int cc =0;
        while(cc < MemorySize()){
            if(Header->Name == Memory[cc]->Type && 
                Header->Type == Memory[cc]->Type && 
                Header->tContext.ProgramID == Memory[cc]->tContext.ProgramID){
                    Free_(Memory[cc], (void *){0b00000000});
            }
            ++cc;
        }
        Header = (header_t){
            .Data = (uint8_t[PageSize]){},
            .Name = (uint8_t[NameIDSize]){},
            .Type = (uint8_t[TypeIDSize]){},
            .Checks = -1,
            .Pointed = false,
            .tContext = (bcontext_t){
                .WholeObjectSize = 0,
                .ProgramID = (uint8_t[ProgramIDSize]){},
                .ID = (uint8_t[IDSize]){},
            },
            .NextBlock = NULL,
            .LastBlock = NULL
        };
    }
}


void ProgramKill(uint8_t ProgramID[ProgramIDSize]){
    int cc =0;
    while(cc < MemorySize()){
        Free_(Memory[cc], (void *){0b00000000});
        ++cc;
    }
}