bits 32

global _LoadGDT
;   void LoadGDT(gdtDESC_t *)
_LoadGDT:
    push ebp
    mov bp, sp
    push eax
    mov eax, [bp + 8]   ; Not sure if correct
    lgdt [eax]
    pop eax
    leave
    ret
