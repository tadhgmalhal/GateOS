global idt_flush

extern isr_handler

; macro to define an ISR stub without an error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0         ; push dummy error code
    push byte %1        ; push interrupt number
    jmp isr_common_stub
%endmacro

; macro to define an ISR stub with an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push byte %1        ; CPU already pushed error code, just push interrupt number
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

isr_common_stub:
    pusha               ; push eax, ecx, edx, ebx, esp, ebp, esi, edi
    mov ax, ds
    push eax            ; push data segment

    mov ax, 0x10        ; load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; push pointer to registers as argument
    call isr_handler
    add esp, 4          ; clean up argument

    pop eax             ; restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; restore general purpose registers
    add esp, 8          ; clean up int_no and err_code
    iret                ; return from interrupt