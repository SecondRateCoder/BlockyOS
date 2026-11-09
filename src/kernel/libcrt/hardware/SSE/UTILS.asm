bits 64
default rel

section .text

; ==============================================================================
; Shared helpers
;   * Every function returns bool in eax (1 = ok, 0 = rejected).
;   * Legacy-SSE instructions with a memory operand fault (#GP) unless the
;     address is 16-byte aligned, so ENTER_PTR validates before touching memory.
;     VEX-encoded (AVX) arithmetic has no such requirement, so it only gets the
;     NULL check.
; ==============================================================================
SSE_ReturnFalse:
    xor eax, eax
    ret

%macro ENTER_PTR 1                  ; %1 = alignment mask (0 = NULL check only)
    test rdi, rdi
    jz SSE_ReturnFalse
  %if %1 != 0
    test dil, %1
    jnz SSE_ReturnFalse
  %endif
%endmacro

%macro RET_TRUE 0
    mov eax, 1
    ret
%endmacro

; ==============================================================================
; MASTER GENERATOR MACRO (per register number %1)
; ==============================================================================
%macro GENERATE_REGISTER_SUITE 1

; ---------------------------- SINGLE-PRECISION -------------------------------
global MovAlignedFloatsToXmm%1, MovUAlignedFloatsToXmm%1
global MovAlignedFloatsFromXmm%1, MovUAlignedFloatsFromXmm%1
global AddFloatXmm%1, SubFloatXmm%1, MulFloatXmm%1, DivFloatXmm%1
global SqrtFloatXmm%1
global AndFloatXmm%1, OrFloatXmm%1, XorFloatXmm%1, ClearFloatXmm%1
global EqualToFloatXmm%1, LessThanFloatXmm%1
global NotEqualToFloatXmm%1, GreaterThanFloatXmm%1
global BroadcastFloatLane0Xmm%1, InterleaveLowFloatXmm%1, ConvertFloatToInt32TruncXmm%1

MovAlignedFloatsToXmm%1:
    ENTER_PTR 15
    movaps xmm%1, [rdi]
    RET_TRUE
MovUAlignedFloatsToXmm%1:
    ENTER_PTR 0
    movups xmm%1, [rdi]
    RET_TRUE
MovAlignedFloatsFromXmm%1:
    ENTER_PTR 15
    movaps [rdi], xmm%1
    RET_TRUE
MovUAlignedFloatsFromXmm%1:
    ENTER_PTR 0
    movups [rdi], xmm%1
    RET_TRUE

AddFloatXmm%1:
    ENTER_PTR 15
    addps xmm%1, [rdi]
    RET_TRUE
SubFloatXmm%1:
    ENTER_PTR 15
    subps xmm%1, [rdi]
    RET_TRUE
MulFloatXmm%1:
    ENTER_PTR 15
    mulps xmm%1, [rdi]
    RET_TRUE
DivFloatXmm%1:
    ENTER_PTR 15
    divps xmm%1, [rdi]
    RET_TRUE
SqrtFloatXmm%1:
    ENTER_PTR 15
    sqrtps xmm%1, [rdi]
    RET_TRUE
AndFloatXmm%1:
    ENTER_PTR 15
    andps xmm%1, [rdi]
    RET_TRUE
OrFloatXmm%1:
    ENTER_PTR 15
    orps xmm%1, [rdi]
    RET_TRUE
XorFloatXmm%1:
    ENTER_PTR 15
    xorps xmm%1, [rdi]
    RET_TRUE
ClearFloatXmm%1:
    xorps xmm%1, xmm%1
    RET_TRUE

EqualToFloatXmm%1:
    ENTER_PTR 15
    cmpps xmm%1, [rdi], 0
    RET_TRUE
LessThanFloatXmm%1:
    ENTER_PTR 15
    cmpps xmm%1, [rdi], 1
    RET_TRUE
NotEqualToFloatXmm%1:
    ENTER_PTR 15
    cmpps xmm%1, [rdi], 4
    RET_TRUE
GreaterThanFloatXmm%1:
    ENTER_PTR 15
    cmpps xmm%1, [rdi], 6
    RET_TRUE

BroadcastFloatLane0Xmm%1:
    shufps xmm%1, xmm%1, 0x00
    RET_TRUE
InterleaveLowFloatXmm%1:
    ENTER_PTR 15
    unpcklps xmm%1, [rdi]
    RET_TRUE
ConvertFloatToInt32TruncXmm%1:
    ENTER_PTR 15
    cvttps2dq xmm%1, [rdi]
    RET_TRUE

; ------------------------------ PACKED INTEGERS ------------------------------
global MovAlignedIntegersToXmm%1, MovUAlignedIntegersToXmm%1
global MovAlignedIntegersFromXmm%1, MovUAlignedIntegersFromXmm%1
global AddIntegerXmm%1, SubIntegerXmm%1, MulIntegerXmm%1
global AndIntegerXmm%1, OrIntegerXmm%1, XorIntegerXmm%1, ClearIntegerXmm%1

MovAlignedIntegersToXmm%1:
    ENTER_PTR 15
    movdqa xmm%1, [rdi]
    RET_TRUE
MovUAlignedIntegersToXmm%1:
    ENTER_PTR 0
    movdqu xmm%1, [rdi]
    RET_TRUE
MovAlignedIntegersFromXmm%1:
    ENTER_PTR 15
    movdqa [rdi], xmm%1
    RET_TRUE
MovUAlignedIntegersFromXmm%1:
    ENTER_PTR 0
    movdqu [rdi], xmm%1
    RET_TRUE

AddIntegerXmm%1:
    ENTER_PTR 15
    paddd xmm%1, [rdi]
    RET_TRUE
SubIntegerXmm%1:
    ENTER_PTR 15
    psubd xmm%1, [rdi]
    RET_TRUE
MulIntegerXmm%1:                    ; pmulld requires SSE4.1
    ENTER_PTR 15
    pmulld xmm%1, [rdi]
    RET_TRUE
AndIntegerXmm%1:
    ENTER_PTR 15
    pand xmm%1, [rdi]
    RET_TRUE
OrIntegerXmm%1:
    ENTER_PTR 15
    por xmm%1, [rdi]
    RET_TRUE
XorIntegerXmm%1:
    ENTER_PTR 15
    pxor xmm%1, [rdi]
    RET_TRUE
ClearIntegerXmm%1:
    pxor xmm%1, xmm%1
    RET_TRUE

; ------------------------------ DOUBLE-PRECISION -----------------------------
global MovAlignedDoublesToXmm%1, MovUAlignedDoublesToXmm%1
global MovAlignedDoublesFromXmm%1, MovUAlignedDoublesFromXmm%1
global AddDoubleXmm%1, SubDoubleXmm%1, MulDoubleXmm%1, DivDoubleXmm%1
global SqrtDoubleXmm%1

MovAlignedDoublesToXmm%1:
    ENTER_PTR 15
    movapd xmm%1, [rdi]
    RET_TRUE
MovUAlignedDoublesToXmm%1:
    ENTER_PTR 0
    movupd xmm%1, [rdi]
    RET_TRUE
MovAlignedDoublesFromXmm%1:
    ENTER_PTR 15
    movapd [rdi], xmm%1
    RET_TRUE
MovUAlignedDoublesFromXmm%1:
    ENTER_PTR 0
    movupd [rdi], xmm%1
    RET_TRUE

AddDoubleXmm%1:
    ENTER_PTR 15
    addpd xmm%1, [rdi]
    RET_TRUE
SubDoubleXmm%1:
    ENTER_PTR 15
    subpd xmm%1, [rdi]
    RET_TRUE
MulDoubleXmm%1:
    ENTER_PTR 15
    mulpd xmm%1, [rdi]
    RET_TRUE
DivDoubleXmm%1:
    ENTER_PTR 15
    divpd xmm%1, [rdi]
    RET_TRUE
SqrtDoubleXmm%1:
    ENTER_PTR 15
    sqrtpd xmm%1, [rdi]
    RET_TRUE

; ------------------------------- 256-BIT AVX (YMM) ---------------------------
global MovAlignedVectorsToYmm%1, MovUAlignedVectorsToYmm%1
global MovAlignedVectorsFromYmm%1, MovUAlignedVectorsFromYmm%1
global AddVectorYmm%1, SubVectorYmm%1, MulVectorYmm%1

MovAlignedVectorsToYmm%1:
    ENTER_PTR 31
    vmovaps ymm%1, [rdi]
    RET_TRUE
MovUAlignedVectorsToYmm%1:
    ENTER_PTR 0
    vmovups ymm%1, [rdi]
    RET_TRUE
MovAlignedVectorsFromYmm%1:
    ENTER_PTR 31
    vmovaps [rdi], ymm%1
    RET_TRUE
MovUAlignedVectorsFromYmm%1:
    ENTER_PTR 0
    vmovups [rdi], ymm%1
    RET_TRUE

AddVectorYmm%1:
    ENTER_PTR 0
    vaddps ymm%1, ymm%1, [rdi]
    RET_TRUE
SubVectorYmm%1:
    ENTER_PTR 0
    vsubps ymm%1, ymm%1, [rdi]
    RET_TRUE
MulVectorYmm%1:
    ENTER_PTR 0
    vmulps ymm%1, ymm%1, [rdi]
    RET_TRUE

%endmacro

; ==============================================================================
; INSTANTIATE REGISTERS 0 TO 15
; %[i] forces the counter to be expanded to its number *before* it is pasted
; into names such as xmm%1 / MovAlignedFloatsToXmm%1.
; ==============================================================================
%assign i 0
%rep 16
    GENERATE_REGISTER_SUITE %[i]
%assign i i+1
%endrep