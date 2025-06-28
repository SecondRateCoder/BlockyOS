#include "Source/OS/Mem-Manager/MemTypes.h"
#include "OS/Functions/Functions.h"

//Pointer
//Back Pointer that is actually used.
//REMARK: Alloca should be the only function to increments this
size_t *Pointer;
//Allocates the desired space for the Process, return NULL if not enough space can be allocated.
//Also points to the first item at the 
header_t *Alloca(size_t Size, uint8_t Name[NameIDSize], uint8_t Type[TypeIDSize], uint8_t ProgramID[ProgramIDSize], bool Protect){
    if(IsPointingFree(Pointer)){
        Memory[*Pointer] = (header_t){
            .Checks = Protect? ProtectedChecks: Un_ProtectedChecks,
            .Data = NULL,
            .LastBlock = NULL,
            .NextBlock = IncTill(Size, Memory[*Pointer]),
            .Name = Name,
            .Type = Type,
            .Pointed = true,
            .tContext = (bcontext_t){
                .ID = AssignID(ProgramID),
                .ProgramID = ProgramID,
                .WholeObjectSize = Size
            },
        };
        Size -= PageSize;
        if(Size >= PageSize){IncTill(Size, Memory[*Pointer]);}
    }else{
        bool Worked =Point_PointerFree(Pointer);
        if(!Worked){return NULL;}else{
            Memory[*Pointer] = (header_t){
                .Checks = Protect? ProtectedChecks: Un_ProtectedChecks,
                .Data = NULL,
                .LastBlock = NULL,
                .NextBlock = IncTill(Size, Memory[*Pointer]),
                .Name = Name,
                .Type = Type,
                .Pointed = true,
                .tContext = (bcontext_t){
                    .ID = AssignID(ProgramID),
                    .ProgramID = ProgramID,
                    .WholeObjectSize = Size
                },
            };
            Size -= PageSize;
            if(Size >= PageSize){IncTill(Size, Memory[*Pointer]);}
        }
    }
    return Memory[*Pointer];
}

header_t *IncTill(size_t Size, header_t RefTo){
    size_t IndexOfNext =0;
    while(Size > 0){
        Size -= PageSize;
        if(!IsPointingFree(Pointer)){
            if(!Point_PointerFree(Pointer)){return NULL;}
        }
        Memory[*Pointer] = (header_t){
            .Checks = RefTo.Checks,
            .LastBlock = &RefTo,
            .Name = RefTo.Name,
            .Type = RefTo.Type,
            .Pointed = true,
            .tContext = (bcontext_t){
                .ID = AssignID(RefTo.tContext.ProgramID),
                .ProgramID = RefTo.tContext.ProgramID,
                .WholeObjectSize = RefTo.tContext.WholeObjectSize,
            }
        };
        if(Size > 0){Memory[*Pointer]->NextBlock = IncTill(Size, RefTo);}else{return Memory[*Pointer];}
    }
}

uint8_t *AssignID(uint8_t ProgramID[ProgramIDSize]){
    Hash_ToEqual(ProgramID, DefaultHashLevel);
    ProgramID[ProgramIDSize -1] += HeaderCount(ProgramID);
    return ProgramID;
}

int HeaderCount(uint8_t ProgramID[ProgramIDSize]){
    int cc =0, Count = 0;
    while(cc < MemorySize()){
        if(Memory[cc]->tContext.ProgramID == ProgramID){Count++;}
        cc++;
    }
    return Count;
}

bool Point_PointerFree(size_t *Pointer_){
    if(Pointer_ > _ram_length){Pointer_ =0;}
    int Retry;
    size_t *MyPointer = GetPointerIndex();
    while(Retry < RetryLimit){
        while(Pointer_ < _ram_length){
            if(Memory[*Pointer_]->Pointed == true || Memory[*Pointer_] == NULL){return true;}
            ++Pointer_;
        }
        Pointer_ =0;
        Retry++;
    }
    return false;
}

void PointAt(header_t Header, WorkDirection dir, size_t *Num){
    switch(dir){
        case ForWard:
            if(Header.NextBlock != NULL){++Num; PointAt(*Header.NextBlock, ForWard, Num);}
            break;
        case BackWard:
            if(Header.LastBlock != NULL){++Num; PointAt(*Header.LastBlock, BackWard, Num);}
            break;
        default:
            break;
    }
}

bool Point_PointerSimilar(header_t RefTo, size_t *Pointer_){
    if(Pointer_ > _ram_length){Pointer_ =0;}
    while(Pointer_ < _ram_length){        
        if(Memory[*Pointer_]->Type == RefTo.Type && Memory[*Pointer_]->Name == RefTo.Name && Memory[*Pointer_]->tContext.ProgramID == RefTo.tContext.ProgramID && Memory[*Pointer_]->tContext.ID != RefTo.tContext.ID){return true;}
        ++Pointer;
    }
    return false;
}

bool IsPointingFree(size_t Pointer_){return Memory[Pointer_]->Pointed == true || Memory[Pointer_] == NULL? true: false;}
bool IsPointingUsed(size_t Pointer_){return !IsPointingFree(Pointer_);}
int MemorySize(){return sizeof(Memory)/sizeof(header_t *);}

void DoOnAllBlocks(Function *f, void *args, header_t *StartFrom, WorkDirection Direction){
    switch (Direction){
        //Find last item, work on from there.
        case ForWard:
            if(StartFrom->LastBlock != NULL){DoOnAllBlocks(f, args, StartFrom->LastBlock, Direction);}else{
                DoOnAllBlocks(f, args, StartFrom, StartBeginning_GoForward);
            }
            break;
        //Find first item, work on from there.
        case BackWard:
            if(StartFrom->NextBlock != NULL){DoOnAllBlocks(f, args, StartFrom->NextBlock, Direction);}else{
                DoOnAllBlocks(f, args, StartFrom, StartEnd_GoBackward);
            }
            break;
        //Find last or first items, work on from there, preffereably from Forward to the Back.
        case Omni_Directional:
            DoOnAllBlocks(f, args, StartFrom->NextBlock, ForWard);
            break;
        // case Omni_Directional:
        //     if(StartFrom->NextBlock != NULL){DoOnAllBlocks(f, args, StartFrom->NextBlock, ForWard);}else
        //     if(StartFrom->LastBlock != NULL && StartFrom->){DoOnAllBlocks(f, args, StartFrom->LastBlock, BackWard);}else
        //     {DoOnAllBlocks(f, args, StartFrom->LastBlock, (StartFrom->LastBlock == NULL && StartFrom->NextBlock != NULL? StartBeginning_GoForward: StartEnd_GoBackward));}
        //     break;
        case StartBeginning_GoForward:
            //Do Logic;
            //If illogical pass, then force into normal running path;
            if(StartFrom->LastBlock != NULL){DoOnAllBlocks(f, args, StartFrom, BackWard);}
            f(StartFrom, args);
            DoOnAllBlocks(f, args, StartFrom->NextBlock, (StartFrom->NextBlock == NULL? Break: StartBeginning_GoForward));
            break;
        case StartEnd_GoBackward:
            //Do Logic;
            //If illogical pass, then force into normal running path;
            if(StartFrom->NextBlock != NULL){DoOnAllBlocks(f, args, StartFrom, ForWard);}
            f(StartFrom, args);
            DoOnAllBlocks(f, args, StartFrom->NextBlock, (StartFrom->NextBlock == NULL? Break: StartEnd_GoBackward));
            break;
        case Break:
            f(StartFrom, args);
            return;
        default:
            break;
    }
}