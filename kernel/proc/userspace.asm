global jump_to_userspace

; void jump_to_userspace(uint32_t eip, uint32_t esp)
; [esp+4] = eip
; [esp+8] = user esp
jump_to_userspace:
    mov ecx, [esp+4]    ; ecx = eip
    mov edx, [esp+8]    ; edx = user esp

    cli

    ; build iret frame
    push 0x23           ; ss
    push edx            ; user esp
    pushf
    pop eax
    or eax, 0x200       ; enable interrupts
    push eax            ; eflags
    push 0x1B           ; cs
    push ecx            ; eip

    ; load user data segments
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    iret