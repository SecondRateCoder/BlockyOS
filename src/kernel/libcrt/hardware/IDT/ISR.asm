bits 64
extern InterruptCallbacks

; For interrupts w/o a CPU error code
%macro isrCallbackNoError 1
global isrCallback%1
isrCallback%1:
    push qword 0    ; Dummy error code (for uniform stack frame layout)
    push qword %1   ; Vector number
    jmp common_isr_stub
%endmacro

; For interrupts w a CPU error code (CPU already pushed error code)
%macro isrCallbackWError 1
global isrCallback%1
isrCallback%1:
    push qword %1   ; Vector number
    jmp common_isr_stub
%endmacro

section .text

common_isr_stub:
    ; Save all general-purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Save vector number before aligning stack
    mov rbx, [rsp + 120]    ; Vector number is above 15 registers (15 * 8 = 120)

    ; Pass InterruptStackFrame pointer as 1st parameter (RDI) in System V ABI
    mov rdi, rsp

    ; Align RSP to 16 bytes for System V ABI compliance
    mov rbp, rsp            ; Save unaligned RSP in RBP (RBP is preserved across C calls)
    and rsp, -16            ; Align RSP to 16-byte boundary

    cld                     ; Clear direction flag for standard C ABI

    ; Call C handler from callback array using the saved vector number
    ; call qword [InterruptCallbacks + (8 * rbx)]
	lea rax, [rel InterruptCallbacks]
	call [rax + (8 * rbx)]

    ; Restore unaligned stack frame pointer
    mov rsp, rbp

    ; Restore registers in reverse order
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Clean up Vector Number and Error Code (or Dummy Error Code)
    add rsp, 16

    ; Return from interrupt
    iretq
isrCallbackNoError 0
isrCallbackNoError 1
isrCallbackNoError 2
isrCallbackNoError 3
isrCallbackNoError 4
isrCallbackNoError 5
isrCallbackNoError 6
isrCallbackNoError 7
isrCallbackWError 8
isrCallbackNoError 9
isrCallbackWError 10
isrCallbackWError 11
isrCallbackWError 12
isrCallbackWError 13
isrCallbackWError 14
isrCallbackNoError 15
isrCallbackNoError 16
isrCallbackWError 17
isrCallbackNoError 18
isrCallbackNoError 19
isrCallbackNoError 20
isrCallbackWError 21
isrCallbackNoError 22
isrCallbackNoError 23
isrCallbackNoError 24
isrCallbackNoError 25
isrCallbackNoError 26
isrCallbackNoError 27
isrCallbackNoError 28
isrCallbackNoError 29
isrCallbackNoError 30
isrCallbackNoError 31
isrCallbackNoError 32
isrCallbackNoError 33
isrCallbackNoError 34
isrCallbackNoError 35
isrCallbackNoError 36
isrCallbackNoError 37
isrCallbackNoError 38
isrCallbackNoError 39
isrCallbackNoError 40
isrCallbackNoError 41
isrCallbackNoError 42
isrCallbackNoError 43
isrCallbackNoError 44
isrCallbackNoError 45
isrCallbackNoError 46
isrCallbackNoError 47
isrCallbackNoError 48
isrCallbackNoError 49
isrCallbackNoError 50
isrCallbackNoError 51
isrCallbackNoError 52
isrCallbackNoError 53
isrCallbackNoError 54
isrCallbackNoError 55
isrCallbackNoError 56
isrCallbackNoError 57
isrCallbackNoError 58
isrCallbackNoError 59
isrCallbackNoError 60
isrCallbackNoError 61
isrCallbackNoError 62
isrCallbackNoError 63
isrCallbackNoError 64
isrCallbackNoError 65
isrCallbackNoError 66
isrCallbackNoError 67
isrCallbackNoError 68
isrCallbackNoError 69
isrCallbackNoError 70
isrCallbackNoError 71
isrCallbackNoError 72
isrCallbackNoError 73
isrCallbackNoError 74
isrCallbackNoError 75
isrCallbackNoError 76
isrCallbackNoError 77
isrCallbackNoError 78
isrCallbackNoError 79
isrCallbackNoError 80
isrCallbackNoError 81
isrCallbackNoError 82
isrCallbackNoError 83
isrCallbackNoError 84
isrCallbackNoError 85
isrCallbackNoError 86
isrCallbackNoError 87
isrCallbackNoError 88
isrCallbackNoError 89
isrCallbackNoError 90
isrCallbackNoError 91
isrCallbackNoError 92
isrCallbackNoError 93
isrCallbackNoError 94
isrCallbackNoError 95
isrCallbackNoError 96
isrCallbackNoError 97
isrCallbackNoError 98
isrCallbackNoError 99
isrCallbackNoError 100
isrCallbackNoError 101
isrCallbackNoError 102
isrCallbackNoError 103
isrCallbackNoError 104
isrCallbackNoError 105
isrCallbackNoError 106
isrCallbackNoError 107
isrCallbackNoError 108
isrCallbackNoError 109
isrCallbackNoError 110
isrCallbackNoError 111
isrCallbackNoError 112
isrCallbackNoError 113
isrCallbackNoError 114
isrCallbackNoError 115
isrCallbackNoError 116
isrCallbackNoError 117
isrCallbackNoError 118
isrCallbackNoError 119
isrCallbackNoError 120
isrCallbackNoError 121
isrCallbackNoError 122
isrCallbackNoError 123
isrCallbackNoError 124
isrCallbackNoError 125
isrCallbackNoError 126
isrCallbackNoError 127
isrCallbackNoError 128
isrCallbackNoError 129
isrCallbackNoError 130
isrCallbackNoError 131
isrCallbackNoError 132
isrCallbackNoError 133
isrCallbackNoError 134
isrCallbackNoError 135
isrCallbackNoError 136
isrCallbackNoError 137
isrCallbackNoError 138
isrCallbackNoError 139
isrCallbackNoError 140
isrCallbackNoError 141
isrCallbackNoError 142
isrCallbackNoError 143
isrCallbackNoError 144
isrCallbackNoError 145
isrCallbackNoError 146
isrCallbackNoError 147
isrCallbackNoError 148
isrCallbackNoError 149
isrCallbackNoError 150
isrCallbackNoError 151
isrCallbackNoError 152
isrCallbackNoError 153
isrCallbackNoError 154
isrCallbackNoError 155
isrCallbackNoError 156
isrCallbackNoError 157
isrCallbackNoError 158
isrCallbackNoError 159
isrCallbackNoError 160
isrCallbackNoError 161
isrCallbackNoError 162
isrCallbackNoError 163
isrCallbackNoError 164
isrCallbackNoError 165
isrCallbackNoError 166
isrCallbackNoError 167
isrCallbackNoError 168
isrCallbackNoError 169
isrCallbackNoError 170
isrCallbackNoError 171
isrCallbackNoError 172
isrCallbackNoError 173
isrCallbackNoError 174
isrCallbackNoError 175
isrCallbackNoError 176
isrCallbackNoError 177
isrCallbackNoError 178
isrCallbackNoError 179
isrCallbackNoError 180
isrCallbackNoError 181
isrCallbackNoError 182
isrCallbackNoError 183
isrCallbackNoError 184
isrCallbackNoError 185
isrCallbackNoError 186
isrCallbackNoError 187
isrCallbackNoError 188
isrCallbackNoError 189
isrCallbackNoError 190
isrCallbackNoError 191
isrCallbackNoError 192
isrCallbackNoError 193
isrCallbackNoError 194
isrCallbackNoError 195
isrCallbackNoError 196
isrCallbackNoError 197
isrCallbackNoError 198
isrCallbackNoError 199
isrCallbackNoError 200
isrCallbackNoError 201
isrCallbackNoError 202
isrCallbackNoError 203
isrCallbackNoError 204
isrCallbackNoError 205
isrCallbackNoError 206
isrCallbackNoError 207
isrCallbackNoError 208
isrCallbackNoError 209
isrCallbackNoError 210
isrCallbackNoError 211
isrCallbackNoError 212
isrCallbackNoError 213
isrCallbackNoError 214
isrCallbackNoError 215
isrCallbackNoError 216
isrCallbackNoError 217
isrCallbackNoError 218
isrCallbackNoError 219
isrCallbackNoError 220
isrCallbackNoError 221
isrCallbackNoError 222
isrCallbackNoError 223
isrCallbackNoError 224
isrCallbackNoError 225
isrCallbackNoError 226
isrCallbackNoError 227
isrCallbackNoError 228
isrCallbackNoError 229
isrCallbackNoError 230
isrCallbackNoError 231
isrCallbackNoError 232
isrCallbackNoError 233
isrCallbackNoError 234
isrCallbackNoError 235
isrCallbackNoError 236
isrCallbackNoError 237
isrCallbackNoError 238
isrCallbackNoError 239
isrCallbackNoError 240
isrCallbackNoError 241
isrCallbackNoError 242
isrCallbackNoError 243
isrCallbackNoError 244
isrCallbackNoError 245
isrCallbackNoError 246
isrCallbackNoError 247
isrCallbackNoError 248
isrCallbackNoError 249
isrCallbackNoError 250
isrCallbackNoError 251
isrCallbackNoError 252
isrCallbackNoError 253
isrCallbackNoError 254
isrCallbackNoError 255
