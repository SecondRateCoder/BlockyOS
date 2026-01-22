bits 32

global _LoadIDT
;   void LoadGDT(gdtDESC_t *)
_LoadIDT:
    push ebp
    mov bp, sp
    push eax
    mov eax, [bp + 8]   ; Not sure if correct
    lidt [eax]
    pop eax
    leave
    ret