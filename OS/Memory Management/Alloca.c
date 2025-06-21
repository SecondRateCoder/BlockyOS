#include "OS/Memory Management/MemoryBlocks.h"

//Srface pointer for functions.
unsigned long long long Pointer;
//Back Pointer that is actually used.
unsigned long long long Pointer_;
//Allocates the desired space for the Process, return NULL if not enough space can be allocated.
header_t *Alloca(size_t Size){

}

#define RetryLimit 5
bool Point_PointerFree(){
    if(Pointer > ram_length){Pointer =0;}
    int Retry;
    while(Retry < RetryLimit){
        while(Pointer < ram_length){
            if(Memory[Pointer]->Pointed == true || Memory[Pointer] == NULL){return true;}
            Pointer++;
        }
        Pointer =0;
        Retry++;
    }
    return false;
}

bool IsPointingFree(){return Memory[Pointer]->Pointed == true || Memory[Pointer] == NULL? true: false;}
bool IsPointingUsed(){return !IsPointingFree();}

bool PointPointerSimilar(header_t RefTo){
    if(Pointer > ram_length){Pointer =0;}
    while(Pointer < ram_length){
        if(Memory[Pointer]->Type == RefTo.Type && Memory[Pointer]->Name == RefTo.Name && Memory[Pointer]->tContext.ProgramID == RefTo.tContext.ProgramID && Memory[cc]->tContext.ID != RefTo.tContext.ID){return true;}
        Pointer++;
    }
    return false;
}

void DoOnAllBlocks(Function f, void *args, header_t StartFrom, WorkDirection Direction){
    
}