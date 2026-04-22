global gdt_flush

gdt_flush:
    mov eax, [esp+4] ; grabbing pointer to gdt_ptr from stack
    lgdt [eax] ; loading into GDTR register

    mov ax, 0x10 ; offsetting data segment
    mov ds, ax ; reloading data segment registers
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush ; reloading with kernel code
.flush:
    ret