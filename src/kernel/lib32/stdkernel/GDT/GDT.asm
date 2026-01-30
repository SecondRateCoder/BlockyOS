bits 32

global _LoadGDT
;   void LoadGDT(gdtDESC_t *, uint16_t, uint16_t)
;   Dont bother making Call Frame
_LoadGDT:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    lgdt [eax]
    ; Reload cs
    mov eax, [ebp + 12]
    push eax
    push .reload_cs
    retf
.reload_cs:
    ; Reload All-Data Segments
    mov eax, [ebp + 16]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    leave
    ret