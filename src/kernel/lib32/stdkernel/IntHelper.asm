bits 32

global _LoadIDT
;   void LoadIDT(idtDESC_t *)
_LoadIDT:
    mov eax, [bp + 8]   ; Not sure if correct
    lidt [eax]
    ret