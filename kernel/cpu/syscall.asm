global syscall_entry
extern syscall_handler

syscall_entry:
    pusha
    push ds

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call syscall_handler
    add esp, 4

    mov [esp+32], eax

    pop ds
    popa

    iret