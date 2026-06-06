MBALIGN     equ  1<<0
MEMINFO     equ  1<<1
FLAGS       equ  MBALIGN | MEMINFO | VIDINFO
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd 0x1BADB002
    dd FLAGS
    dd -(0x1BADB002 + FLAGS)
    ; aout kludge (unused for ELF, pad with zeros)
    dd 0, 0, 0, 0, 0
    ; Video mode request (0 = linear graphics)
    dd 0
    dd 800  ; Width
    dd 600  ; Height
    dd 32   ; Depth

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; 初始化堆疊指標
    mov esp, stack_top
    
    ; 傳遞 multiboot_info (ebx) 與 magic (eax) 給 kernel_main
    push ebx
    push eax
    call kernel_main

    ; 停機迴圈
    cli
.hang:
    hlt
    jmp .hang
