bits 64
section .text

; ==============================================================================
; 1. MASTER GENERATOR MACRO (Per Register Number %1)
; ==============================================================================
%macro GENERATE_REGISTER_SUITE 1

; ------------------------------------------------------------------------------
; SINGLE-PRECISION FLOATS (XMM)
; ------------------------------------------------------------------------------
global MovAlignedFloatsToXmm%1, MovUAlignedFloatsToXmm%1
global MovAlignedFloatsFromXmm%1, MovUAlignedFloatsFromXmm%1
global AddFloatXmm%1, SubFloatXmm%1
global MulFloatXmm%1, DivFloatXmm%1
global SqrtFloatXmm%1
global AndFloatXmm%1, OrFloatXmm%1
global XorFloatXmm%1, ClearFloatXmm%1
global EqualToFloatXmm%1, LessThanFloatXmm%1
global NotEqualToFloatXmm%1, GreaterThanFloatXmm%1
global BroadcastFloatLane0Xmm%1, InterleaveLowFloatXmm%1, ConvertFloatToInt32TruncXmm%1
MovAlignedFloatsToXmm%1:
    test rdi, 15
    jnz .bad_align
    movaps xmm%1, [rdi]
    mov rax, 1
    ret
.bad_align:
    xor rax, rax
    ret
MovUAlignedFloatsToXmm%1:
    movups xmm%1, [rdi]
    mov rax, 1
    ret

MovAlignedFloatsFromXmm%1:
    test rdi, 15
    jnz .bad_align
    movaps [rdi], xmm%1
    mov rax, 1
    ret
.bad_align:
    xor rax, rax
    ret
MovUAlignedFloatsFromXmm%1:
    movups [rdi], xmm%1
    mov rax, 1
    ret

AddFloatXmm%1:
    addps xmm%1, [rdi]
    mov rax, 1
    ret
SubFloatXmm%1:
    subps xmm%1, [rdi]
    mov rax, 1
    ret
MulFloatXmm%1:
    mulps xmm%1, [rdi]
    mov rax, 1
    ret
DivFloatXmm%1:
    divps xmm%1, [rdi]
    mov rax, 1
    ret
SqrtFloatXmm%1:
    sqrtps xmm%1, [rdi]
    mov rax, 1
    ret

AndFloatXmm%1:
    andps xmm%1, [rdi]
    mov rax, 1
    ret
OrFloatXmm%1:
    orps xmm%1, [rdi]
    mov rax, 1
    ret
XorFloatXmm%1:
    xorps xmm%1, [rdi]
    mov rax, 1
    ret
ClearFloatXmm%1:
    xorps xmm%1, xmm%1
    mov rax, 1
    ret

EqualToFloatXmm%1:
    cmpps xmm%1, [rdi], 0
    mov rax, 1
    ret
LessThanFloatXmm%1:
    cmpps xmm%1, [rdi], 1
    mov rax, 1
    ret
NotEqualToFloatXmm%1:
    cmpps xmm%1, [rdi], 4
    mov rax, 1
    ret
GreaterThanFloatXmm%1:
    cmpps xmm%1, [rdi], 6
    mov rax, 1
    ret

BroadcastFloatLane0Xmm%1:
    shufps xmm%1, xmm%1, 0x00
    mov rax, 1
    ret
InterleaveLowFloatXmm%1:
    unpcklps xmm%1, [rdi]
    mov rax, 1
    ret
ConvertFloatToInt32TruncXmm%1:
    cvttps2dq xmm%1, [rdi]
    mov rax, 1
    ret


; ------------------------------------------------------------------------------
; PACKED INTEGERS (XMM)
; ------------------------------------------------------------------------------
global MovAlignedIntegersToXmm%1
global MovUAlignedIntegersToXmm%1
global MovAlignedIntegersFromXmm%1
global MovUAlignedIntegersFromXmm%1
global AddIntegerXmm%1
global SubIntegerXmm%1
global MulIntegerXmm%1
global AndIntegerXmm%1
global OrIntegerXmm%1
global XorIntegerXmm%1
global ClearIntegerXmm%1

MovAlignedIntegersToXmm%1:
    test rdi, 15
    jnz .bad_align
    movdqa xmm%1, [rdi]
    mov rax, 1
    ret
.bad_align:
    xor rax, rax
    ret

MovUAlignedIntegersToXmm%1:
    movdqu xmm%1, [rdi]
    mov rax, 1
    ret

MovAlignedIntegersFromXmm%1:
    test rdi, 15
    jnz .bad_align
    movdqa [rdi], xmm%1
    mov rax, 1
    ret
.bad_align:
    xor rax, rax
    ret

MovUAlignedIntegersFromXmm%1:
    movdqu [rdi], xmm%1
    mov rax, 1
    ret

AddIntegerXmm%1:
    paddd xmm%1, [rdi]
    mov rax, 1
    ret

SubIntegerXmm%1:
    psubd xmm%1, [rdi]
    mov rax, 1
    ret

MulIntegerXmm%1:
    pmulld xmm%1, [rdi]
    mov rax, 1
    ret

AndIntegerXmm%1:
    pand xmm%1, [rdi]
    mov rax, 1
    ret

OrIntegerXmm%1:
    por xmm%1, [rdi]
    mov rax, 1
    ret

XorIntegerXmm%1:
    pxor xmm%1, [rdi]
    mov rax, 1
    ret

ClearIntegerXmm%1:
    pxor xmm%1, xmm%1
    mov rax, 1
    ret


; ------------------------------------------------------------------------------
; DOUBLE-PRECISION FLOATS (XMM)
; ------------------------------------------------------------------------------
global MovAlignedDoublesToXmm%1
global MovUAlignedDoublesToXmm%1
global MovAlignedDoublesFromXmm%1
global MovUAlignedDoublesFromXmm%1
global AddDoubleXmm%1
global SubDoubleXmm%1
global MulDoubleXmm%1
global DivDoubleXmm%1
global SqrtDoubleXmm%1

MovAlignedDoublesToXmm%1:
    test rdi, 15
    jnz .bad_align
    movapd xmm%1, [rdi]
    mov rax, 1
    ret
.bad_align:
    xor rax, rax
    ret

MovUAlignedDoublesToXmm%1:
    movupd xmm%1, [rdi]
    mov rax, 1
    ret

MovAlignedDoublesFromXmm%1:
    test rdi, 15
    jnz .bad_align
    movapd [rdi], xmm%1
    mov rax, 1
    ret
.bad_align:
    xor rax, rax
    ret

MovUAlignedDoublesFromXmm%1:
    movupd [rdi], xmm%1
    mov rax, 1
    ret

AddDoubleXmm%1:
    addpd xmm%1, [rdi]
    mov rax, 1
    ret

SubDoubleXmm%1:
    subpd xmm%1, [rdi]
    mov rax, 1
    ret

MulDoubleXmm%1:
    mulpd xmm%1, [rdi]
    mov rax, 1
    ret

DivDoubleXmm%1:
    divpd xmm%1, [rdi]
    mov rax, 1
    ret

SqrtDoubleXmm%1:
    sqrtpd xmm%1, [rdi]
    mov rax, 1
    ret


; ------------------------------------------------------------------------------
; 256-BIT AVX VECTORS (YMM)
; ------------------------------------------------------------------------------
global MovAlignedVectorsToYmm%1
global MovUAlignedVectorsToYmm%1
global MovAlignedVectorsFromYmm%1
global MovUAlignedVectorsFromYmm%1
global AddVectorYmm%1
global SubVectorYmm%1
global MulVectorYmm%1

MovAlignedVectorsToYmm%1:
    test rdi, 31
    jnz .bad_align
    vmovaps ymm%1, [rdi]
    mov rax, 1
    ret
.bad_align:
    xor rax, rax
    ret

MovUAlignedVectorsToYmm%1:
    vmovups ymm%1, [rdi]
    mov rax, 1
    ret

MovAlignedVectorsFromYmm%1:
    test rdi, 31
    jnz .bad_align
    vmovaps [rdi], ymm%1
    mov rax, 1
    ret
.bad_align:
    xor rax, rax
    ret

MovUAlignedVectorsFromYmm%1:
    vmovups [rdi], ymm%1
    mov rax, 1
    ret

AddVectorYmm%1:
    vaddps ymm%1, ymm%1, [rdi]
    mov rax, 1
    ret

SubVectorYmm%1:
    vsubps ymm%1, ymm%1, [rdi]
    mov rax, 1
    ret

MulVectorYmm%1:
    vmulps ymm%1, ymm%1, [rdi]
    mov rax, 1
    ret

%endmacro


; ==============================================================================
; 2. INSTANTIATE REGISTERS 0 TO 31
; ==============================================================================
%assign i 0
%rep 16
    GENERATE_REGISTER_SUITE i
%assign i i+1
%endrep