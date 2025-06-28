#include "OS/Mem-Manager/MemTypes.h"

typedef enum ProgramState{
    //The process is still in the Process of being loaded into Memory, or it's logic being updated in RAM.
    Loading,
    //The program has been loaded into RAM, even if not fully.
    Loaded, 
    Running, 
    //The Program is waiting, will be returned to running when done.
    NOP,
    //The Program is awaiting the OS' function completion.
    AwaitingOS,
    //The Program is waiting for a thread to complete, a glorified millisecond NOP.
    Awaitingthread,
    //The Program has experienced a grave Exception and is being removed from RAM.
    Decaying,
    //The Program is being un-loaded from RAM.
    Closing,
    //The Program has been unloaded, any Processes that want to call it cannot call it anymore.
    Closed,
    //The Program is a Kernel program, it will/can set it's own ProgramState.
    Kernel,
    //The Program is not a Kernel program, it cannot set it's ProgramState.
    Nill
}ProgramStates;

typedef struct Process{
    uint8_t ProgramID[ProgramIDSize];
    char *ProgramPath;
    header_t *ProgramSet;
    /// @brief These three size_t variables handle how Programs are loaded into Memory.
    size_t InstructionSize, LoadSize, ProgressCounter;
    ProgramStates State[2];
}Process;
