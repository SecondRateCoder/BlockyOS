bits 32

global _LoadGDT
;   void LoadGDT(gdtDESC_t *)
_LoadGDT:
    mov eax, [bp + 8]   ; Not sure if correct
    lgdt [eax]
    ret