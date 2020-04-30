; nasm -f elf64 print.asm
; ld print.o
section .data

section .text
extern main
; ----- Main ----- ;
global _start
_start:

    call    main            ; print string1

    mov  rdi,rax            ; Get RC
    mov  rax,60             ; exit syscall
    syscall                 ; call exit
; ----- End Main ----- ;

