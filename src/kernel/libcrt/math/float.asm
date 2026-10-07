bits 64
default rel

section .text

global __addsf3
global __subsf3
__subsf3:
    btc esi, 31
__addsf3:
    mov eax, edi
    and eax, 0x7FFFFFFF
    jnz .a_not_zero
    mov eax, esi
    ret
.a_not_zero:
    mov eax, esi
    and eax, 0x7FFFFFFF
    jnz .b_not_zero
    mov eax, edi
    ret
.b_not_zero:
    mov eax, edi
    xor eax, esi
    js .different_signs

    ; --- SAME SIGNS ---
    mov eax, edi
    and eax, 0x7FFFFFFF
    mov ecx, esi
    and ecx, 0x7FFFFFFF
    cmp eax, ecx
    jae .add_no_swap
    xchg edi, esi
.add_no_swap:
    mov eax, edi
    shr eax, 23
    and eax, 0xFF        ; Exp A
    mov ecx, esi
    shr ecx, 23
    and ecx, 0xFF        ; Exp B

    mov r8d, edi
    and r8d, 0x7FFFFF
    or r8d, 0x800000     ; Mantissa A
    mov r9d, esi
    and r9d, 0x7FFFFF
    or r9d, 0x800000     ; Mantissa B

    sub eax, ecx         ; Exp diff
    mov ecx, eax
    cmp ecx, 31
    jbe .add_shift_ok
    mov ecx, 31
.add_shift_ok:
    shr r9d, cl          ; Shift Mantissa B right

    mov eax, edi
    shr eax, 23
    and eax, 0xFF        ; Re-load Exp A

    add r8d, r9d         ; Add mantissas
    test r8d, 0x01000000 ; Check overflow beyond bit 23
    jz .add_no_overflow
    shr r8d, 1
    inc eax
.add_no_overflow:
    and r8d, 0x7FFFFF
    mov ecx, edi
    and ecx, -0x80000000 ; Sign bit
    shl eax, 23
    or eax, ecx
    or eax, r8d
    ret
.different_signs:
    ; --- DIFFERENT SIGNS ---
    mov eax, edi
    and eax, 0x7FFFFFFF
    mov ecx, esi
    and ecx, 0x7FFFFFFF
    cmp eax, ecx
    jne .diff_not_equal
    xor eax, eax
    ret
.diff_not_equal:
    jae .sub_no_swap
    xchg edi, esi
.sub_no_swap:
    mov eax, edi
    shr eax, 23
    and eax, 0xFF        ; Exp A
    mov ecx, esi
    shr ecx, 23
    and ecx, 0xFF        ; Exp B

    mov r8d, edi
    and r8d, 0x7FFFFF
    or r8d, 0x800000     ; Mantissa A
    mov r9d, esi
    and r9d, 0x7FFFFF
    or r9d, 0x800000     ; Mantissa B

    sub eax, ecx         ; Exp diff
    mov ecx, eax
    cmp ecx, 31
    jbe .sub_shift_ok
    mov ecx, 31
.sub_shift_ok:
    shr r9d, cl          ; Shift Mantissa B right

    mov eax, edi
    shr eax, 23
    and eax, 0xFF        ; Re-load Exp A

    sub r8d, r9d         ; Subtract mantissas

.sub_norm_loop:
    test r8d, 0x800000
    jnz .sub_norm_done
    shl r8d, 1
    dec eax
    jmp .sub_norm_loop
.sub_norm_done:
    and r8d, 0x7FFFFF
    mov ecx, edi
    and ecx, -0x80000000 ; Sign bit
    shl eax, 23
    or eax, ecx
    or eax, r8d
    ret

global __mulsf3
__mulsf3:
    mov eax, edi
    and eax, 0x7FFFFFFF
    jz .mul_zero
    mov eax, esi
    and eax, 0x7FFFFFFF
    jz .mul_zero

    mov r10d, edi
    xor r10d, esi
    and r10d, -0x80000000 ; Result sign

    mov eax, edi
    shr eax, 23
    and eax, 0xFF
    mov ecx, esi
    shr ecx, 23
    and ecx, 0xFF
    add eax, ecx
    sub eax, 127        ; E_res = Ea + Eb - 127

    mov r8, rdi
    and r8, 0x7FFFFF
    or r8, 0x800000
    mov r9, rsi
    and r9, 0x7FFFFF
    or r9, 0x800000

    imul r8, r9         ; 64-bit product of 24-bit mantissas

    bt r8, 47           ; Check bit 47 (0x800000000000)
    jnc .mul_no_overflow
    shr r8, 24
    inc eax
    jmp .mul_pack
.mul_no_overflow:
    shr r8, 23
.mul_pack:
    and r8d, 0x7FFFFF
    shl eax, 23
    or eax, r10d
    or eax, r8d
    ret
.mul_zero:
    mov eax, edi
    xor eax, esi
    and eax, -0x80000000
    ret


global __divsf3
__divsf3:
    mov eax, edi
    and eax, 0x7FFFFFFF
    jz .div_zero

    mov r10d, edi
    xor r10d, esi
    and r10d, -0x80000000 ; Sign

    mov r11d, edi
    shr r11d, 23
    and r11d, 0xFF
    mov ecx, esi
    shr ecx, 23
    and ecx, 0xFF
    sub r11d, ecx
    add r11d, 127       ; Exp = Ea - Eb + 127

    mov r8, rdi
    and r8, 0x7FFFFF
    or r8, 0x800000
    mov r9, rsi
    and r9, 0x7FFFFF
    or r9, 0x800000

    cmp r8, r9
    jae .div_no_shift
    shl r8, 24
    dec r11d
    jmp .div_do
.div_no_shift:
    shl r8, 23
.div_do:
    xor rdx, rdx
    mov rax, r8
    div r9              ; RAX = quotient

    and eax, 0x7FFFFF
    shl r11d, 23
    or eax, r10d
    or eax, r11d
    ret
.div_zero:
    mov eax, edi
    xor eax, esi
    and eax, -0x80000000
    ret


global __eqsf2
__eqsf2:
    mov eax, edi
    or eax, esi
    and eax, 0x7FFFFFFF
    jz .eq_zero
    cmp edi, esi
    je .eq_zero
    mov eax, 1
    ret
.eq_zero:
    xor eax, eax
    ret


global __ltsf2
__ltsf2:
    mov eax, edi
    or eax, esi
    and eax, 0x7FFFFFFF
    jz .lt_equal

    mov eax, edi
    mov ecx, esi
    shr eax, 31
    shr ecx, 31
    cmp eax, ecx
    jne .lt_diff_signs

    test eax, eax
    jnz .lt_both_neg
    cmp edi, esi
    jb .lt_true
    jmp .lt_false
.lt_both_neg:
    cmp edi, esi
    ja .lt_true
    jmp .lt_false
.lt_diff_signs:
    test eax, eax
    jnz .lt_true
.lt_false:
.lt_equal:
    xor eax, eax
    ret
.lt_true:
    mov eax, -1
    ret


global __lesf2
__lesf2:
    mov eax, edi
    or eax, esi
    and eax, 0x7FFFFFFF
    jz .le_true

    cmp edi, esi
    je .le_true

    mov eax, edi
    mov ecx, esi
    shr eax, 31
    shr ecx, 31
    cmp eax, ecx
    jne .le_diff_signs

    test eax, eax
    jnz .le_both_neg
    cmp edi, esi
    jbe .le_true
    jmp .le_false
.le_both_neg:
    cmp edi, esi
    jae .le_true
    jmp .le_false
.le_diff_signs:
    test eax, eax
    jnz .le_true
.le_false:
    mov eax, 1
    ret
.le_true:
    xor eax, eax
    ret


global __gtsf2
__gtsf2:
    mov eax, edi
    or eax, esi
    and eax, 0x7FFFFFFF
    jz .gt_false

    mov eax, edi
    mov ecx, esi
    shr eax, 31
    shr ecx, 31
    cmp eax, ecx
    jne .gt_diff_signs

    test eax, eax
    jnz .gt_both_neg
    cmp edi, esi
    ja .gt_true
    jmp .gt_false
.gt_both_neg:
    cmp edi, esi
    jb .gt_true
    jmp .gt_false
.gt_diff_signs:
    test eax, eax
    jz .gt_true
.gt_false:
    xor eax, eax
    ret
.gt_true:
    mov eax, 1
    ret

global __adddf3
global __subdf3
__subdf3:
    btc rsi, 63
__adddf3:
    mov r11, 0x7FFFFFFFFFFFFFFF
    mov rax, rdi
    and rax, r11
    jnz .df_a_not_zero
    mov rax, rsi
    ret
.df_a_not_zero:
    mov rax, rsi
    and rax, r11
    jnz .df_b_not_zero
    mov rax, rdi
    ret
.df_b_not_zero:
    mov rax, rdi
    xor rax, rsi
    js .df_diff_signs

    ; --- SAME SIGNS ---
    mov rax, rdi
    and rax, r11
    mov rcx, rsi
    and rcx, r11
    cmp rax, rcx
    jae .df_add_no_swap
    xchg rdi, rsi
.df_add_no_swap:
    mov rax, rdi
    shr rax, 52
    and rax, 0x7FF       ; Exp A
    mov rcx, rsi
    shr rcx, 52
    and rcx, 0x7FF       ; Exp B

    mov r8, rdi
    mov r10, 0x000FFFFFFFFFFFFF
    and r8, r10
    mov r11, 0x0010000000000000
    or r8, r11           ; Mantissa A
    mov r9, rsi
    and r9, r10
    or r9, r11           ; Mantissa B

    sub rax, rcx         ; Exp diff
    mov rcx, rax
    cmp rcx, 63
    jbe .df_add_shift_ok
    mov rcx, 63
.df_add_shift_ok:
    shr r9, cl           ; Shift Mantissa B right

    mov rax, rdi
    shr rax, 52
    and rax, 0x7FF       ; Re-load Exp A

    add r8, r9           ; Add mantissas
	mov rcx, 0x0020000000000000
    test r8, rcx
    jz .df_add_no_overflow
    shr r8, 1
    inc rax
.df_add_no_overflow:
    and r8, r10
    mov rcx, rdi
    mov r11, -0x8000000000000000
    and rcx, r11         ; Sign bit
    shl rax, 52
    or rax, rcx
    or rax, r8
    ret
.df_diff_signs:
    mov rax, rdi
    and rax, r11
    mov rcx, rsi
    and rcx, r11
    cmp rax, rcx
    jne .df_diff_not_equal
    xor rax, rax
    ret
.df_diff_not_equal:
    jae .df_sub_no_swap
    xchg rdi, rsi
.df_sub_no_swap:
    mov rax, rdi
    shr rax, 52
    and rax, 0x7FF       ; Exp A
    mov rcx, rsi
    shr rcx, 52
    and rcx, 0x7FF       ; Exp B

    mov r8, rdi
    mov r10, 0x000FFFFFFFFFFFFF
    and r8, r10
    mov r11, 0x0010000000000000
    or r8, r11           ; Mantissa A
    mov r9, rsi
    and r9, r10
    or r9, r11           ; Mantissa B

    sub rax, rcx         ; Exp diff
    mov rcx, rax
    cmp rcx, 63
    jbe .df_sub_shift_ok
    mov rcx, 63
.df_sub_shift_ok:
    shr r9, cl

    mov rax, rdi
    shr rax, 52
    and rax, 0x7FF       ; Re-load Exp A

    sub r8, r9           ; Subtract mantissas
.df_sub_norm_loop:
	mov rcx, 0x0010000000000000
    test r8, rcx
    jnz .df_sub_norm_done
    shl r8, 1
    dec rax
    jmp .df_sub_norm_loop
.df_sub_norm_done:
    and r8, r10
    mov rcx, rdi
    mov r11, -0x8000000000000000
    and rcx, r11         ; Sign bit
    shl rax, 52
    or rax, rcx
    or rax, r8
    ret


global __muldf3
__muldf3:
    mov r8, 0x7FFFFFFFFFFFFFFF
    mov rax, rdi
    and rax, r8
    jz .df_mul_zero
    mov rax, rsi
    and rax, r8
    jz .df_mul_zero

    mov r10, rdi
    xor r10, rsi
    mov r11, -0x8000000000000000
    and r10, r11         ; Sign

    mov r11, rdi
    shr r11, 52
    and r11, 0x7FF
    mov rcx, rsi
    shr rcx, 52
    and rcx, 0x7FF
    add r11, rcx
    sub r11, 1023        ; Exp

    mov r8, rdi
    mov r12, 0x000FFFFFFFFFFFFF
    and r8, r12
	mov rcx, 0x0010000000000000
    or r8, rcx
    mov r9, rsi
    and r9, r12
	mov rcx, 0x0010000000000000
    or r9, rcx

    mov rax, r8
    mul r9               ; RDX:RAX = R8 * R9

    bt rdx, 41           ; Check bit 41 (0x20000000000)
    jnc .df_mul_no_overflow
    shrd rax, rdx, 53
    inc r11
    jmp .df_mul_pack
.df_mul_no_overflow:
    shrd rax, rdx, 52
.df_mul_pack:
    and rax, r12         ; 52-bit mantissa
    shl r11, 52
    or rax, r10          ; Sign
    or rax, r11          ; Exp
    ret
.df_mul_zero:
    mov rax, rdi
    xor rax, rsi
    mov r8, -0x8000000000000000
    and rax, r8
    ret


global __divdf3
__divdf3:
    mov r8, 0x7FFFFFFFFFFFFFFF
    mov rax, rdi
    and rax, r8
    jz .df_div_zero

    mov r10, rdi
    xor r10, rsi
    mov r11, -0x8000000000000000
    and r10, r11         ; Sign

    mov r11, rdi
    shr r11, 52
    and r11, 0x7FF
    mov rcx, rsi
    shr rcx, 52
    and rcx, 0x7FF
    sub r11, rcx
    add r11, 1023        ; Exp

    mov r8, rdi
    mov r12, 0x000FFFFFFFFFFFFF
    and r8, r12
	mov rcx, 0x0010000000000000
    or r8, rcx
    mov r9, rsi
    and r9, r12
    or r9, rcx

    cmp r8, r9
    jae .df_div_no_shift
    mov rdx, r8
    xor eax, eax
    shld rdx, rax, 53
    dec r11
    jmp .df_div_do
.df_div_no_shift:
    mov rdx, r8
    xor eax, eax
    shld rdx, rax, 52
.df_div_do:
    div r9               ; RAX = quotient

    and rax, r12
    shl r11, 52
    or rax, r10
    or rax, r11
    ret
.df_div_zero:
    mov rax, rdi
    xor rax, rsi
    mov r8, -0x8000000000000000
    and rax, r8
    ret


global __eqdf2
__eqdf2:
    mov rax, rdi
    or rax, rsi
    mov r8, 0x7FFFFFFFFFFFFFFF
    and rax, r8
    jz .df_eq_zero
    cmp rdi, rsi
    je .df_eq_zero
    mov eax, 1
    ret
.df_eq_zero:
    xor eax, eax
    ret


global __ltdf2
__ltdf2:
    mov rax, rdi
    or rax, rsi
    mov r8, 0x7FFFFFFFFFFFFFFF
    and rax, r8
    jz .df_lt_equal

    mov rax, rdi
    mov rcx, rsi
    shr rax, 63
    shr rcx, 63
    cmp rax, rcx
    jne .df_lt_diff_signs

    test rax, rax
    jnz .df_lt_both_neg
    cmp rdi, rsi
    jb .df_lt_true
    jmp .df_lt_false
.df_lt_both_neg:
    cmp rdi, rsi
    ja .df_lt_true
    jmp .df_lt_false
.df_lt_diff_signs:
    test rax, rax
    jnz .df_lt_true
.df_lt_false:
.df_lt_equal:
    xor eax, eax
    ret
.df_lt_true:
    mov eax, -1
    ret


global __ledf2
__ledf2:
    mov rax, rdi
    or rax, rsi
    mov r8, 0x7FFFFFFFFFFFFFFF
    and rax, r8
    jz .df_le_true

    cmp rdi, rsi
    je .df_le_true

    mov rax, rdi
    mov rcx, rsi
    shr rax, 63
    shr rcx, 63
    cmp rax, rcx
    jne .df_le_diff_signs

    test rax, rax
    jnz .df_le_both_neg
    cmp rdi, rsi
    jbe .df_le_true
    jmp .df_le_false
.df_le_both_neg:
    cmp rdi, rsi
    jae .df_le_true
    jmp .df_le_false
.df_le_diff_signs:
    test rax, rax
    jnz .df_le_true
.df_le_false:
    mov eax, 1
    ret
.df_le_true:
    xor eax, eax
    ret


global __gtdf2
__gtdf2:
    mov rax, rdi
    or rax, rsi
    mov r8, 0x7FFFFFFFFFFFFFFF
    and rax, r8
    jz .df_gt_false

    mov rax, rdi
    mov rcx, rsi
    shr rax, 63
    shr rcx, 63
    cmp rax, rcx
    jne .df_gt_diff_signs

    test rax, rax
    jnz .df_gt_both_neg
    cmp rdi, rsi
    ja .df_gt_true
    jmp .df_gt_false
.df_gt_both_neg:
    cmp rdi, rsi
    jb .df_gt_true
    jmp .df_gt_false
.df_gt_diff_signs:
    test rax, rax
    jz .df_gt_true
.df_gt_false:
    xor eax, eax
    ret
.df_gt_true:
    mov eax, 1
    ret

global __gedf2
__gedf2:
    mov rax, rdi
    or rax, rsi
    shl rax, 1
    jz .df_ge_true       ; +0 == -0 => true (A >= B)

    cmp rdi, rsi
    je .df_ge_true       ; Bitwise equal => true

    mov rax, rdi
    mov rcx, rsi
    shr rax, 63
    shr rcx, 63
    cmp rax, rcx
    jne .df_ge_diff_signs

    test rax, rax
    jnz .df_ge_both_neg
    cmp rdi, rsi
    jae .df_ge_true
    jmp .df_ge_false

.df_ge_both_neg:
    cmp rdi, rsi
    jbe .df_ge_true
    jmp .df_ge_false

.df_ge_diff_signs:
    test rax, rax
    jz .df_ge_true       ; A is positive, B is negative => A > B

.df_ge_false:
    mov eax, -1          ; < 0 indicates false (A < B)
    ret
.df_ge_true:
    xor eax, eax         ; >= 0 indicates true (A >= B)
    ret

global __floatsisf
__floatsisf:
    test edi, edi
    jnz .sisf_not_zero
    xor eax, eax
    ret
.sisf_not_zero:
    mov r8d, edi
    and r8d, -0x80000000 ; Sign
    mov eax, edi
    jns .sisf_pos
    neg eax              ; Magnitude
.sisf_pos:
    bsr ecx, eax         ; Highest set bit (0..31)
    mov edx, ecx
    add edx, 127         ; Biased exponent

    mov r9d, 23
    sub r9d, ecx
    jge .sisf_shift_left
    neg r9d
    mov ecx, r9d
    shr eax, cl
    jmp .sisf_pack
.sisf_shift_left:
    mov ecx, r9d
    shl eax, cl
.sisf_pack:
    and eax, 0x7FFFFF    ; Mantissa
    shl edx, 23
    or eax, r8d          ; Sign
    or eax, edx          ; Exp
    ret


global __floatunsisf
__floatunsisf:
    test edi, edi
    jnz .unsisf_not_zero
    xor eax, eax
    ret
.unsisf_not_zero:
    mov eax, edi
    bsr ecx, eax
    mov edx, ecx
    add edx, 127         ; Biased exponent

    mov r9d, 23
    sub r9d, ecx
    jge .unsisf_shift_left
    neg r9d
    mov ecx, r9d
    shr eax, cl
    jmp .unsisf_pack
.unsisf_shift_left:
    mov ecx, r9d
    shl eax, cl
.unsisf_pack:
    and eax, 0x7FFFFF
    shl edx, 23
    or eax, edx
    ret


global __floatsidf
__floatsidf:
    test edi, edi
    jnz .sidf_not_zero
    xor rax, rax
    ret
.sidf_not_zero:
    mov r8d, edi
    and r8d, -0x80000000
    shl r8, 32           ; Sign bit at bit 63
    mov eax, edi
    jns .sidf_pos
    neg eax              ; Magnitude
.sidf_pos:
    bsr ecx, eax
    mov edx, ecx
    add edx, 1023        ; Biased exponent for double

    mov r9d, 52
    sub r9d, ecx
    mov ecx, r9d
    shl rax, cl          ; Align mantissa to bit 52
    
    mov rcx, 0x000FFFFFFFFFFFFF
    and rax, rcx
    shl rdx, 52          ; Exp
    or rax, r8           ; Sign
    or rax, rdx          ; Exp
    ret


global __floatundidf
__floatundidf:
    test rdi, rdi
    jnz .undidf_not_zero
    xor rax, rax
    ret
.undidf_not_zero:
    mov rax, rdi
    bsr rcx, rax         ; Highest set bit (0..63)
    mov rdx, rcx
    add rdx, 1023        ; Biased exponent

    mov r9, 52
    sub r9, rcx
    jge .undidf_shift_left
    neg r9
    mov rcx, r9
    shr rax, cl
    jmp .undidf_pack
.undidf_shift_left:
    mov rcx, r9
    shl rax, cl
.undidf_pack:
    mov rcx, 0x000FFFFFFFFFFFFF
    and rax, rcx
    shl rdx, 52
    or rax, rdx
    ret


global __fixsfsi
__fixsfsi:
    mov ecx, edi
    shr ecx, 23
    and ecx, 0xFF        ; Exponent
    sub ecx, 127         ; True exponent

    js .fix_zero         ; Exp < 0 => integer part is 0

    cmp ecx, 31
    jge .fix_zero        ; Overflow or invalid

    mov eax, edi
    and eax, 0x7FFFFF
    or eax, 0x800000     ; Restore implicit 1

    mov edx, 23
    sub edx, ecx
    jge .fix_shift_right
    neg edx
    mov ecx, edx
    shl eax, cl
    jmp .fix_apply_sign
.fix_shift_right:
    mov ecx, edx
    shr eax, cl
.fix_apply_sign:
    test edi, -0x80000000
    jz .fix_done
    neg eax
.fix_done:
    ret
.fix_zero:
    xor eax, eax
    ret


global __truncdfsf2
__truncdfsf2:
    mov r8, rdi
    shr r8, 63           ; Sign (1 bit)

    mov rcx, rdi
    shr rcx, 52
    and rcx, 0x7FF       ; Double exp (11 bits)

    test rcx, rcx
    jz .trunc_zero

    sub rcx, 1023
    add rcx, 127         ; Single exp (8 bits)
    cmp rcx, 255
    jge .trunc_zero      ; Overflow
    cmp rcx, 0
    jle .trunc_zero      ; Underflow

    mov rax, rdi
    mov r9, 0x000FFFFFFFFFFFFF
    and rax, r9          ; 52-bit mantissa
    shr rax, 29          ; Truncate to 23 bits

    shl r8d, 31
    shl ecx, 23
    or eax, r8d
    or eax, ecx
    ret
.trunc_zero:
    mov rax, rdi
    shr rax, 32
    and eax, -0x80000000
    ret