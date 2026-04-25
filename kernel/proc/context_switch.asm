global context_switch

; void context_switch(uint32_t *old_esp, uint32_t new_esp, uint32_t page_dir)
context_switch:
    push ebp
    push ebx
    push esi
    push edi

    ; save old esp
    mov eax, [esp+20]
    mov [eax], esp

    ; load new_esp and page_dir BEFORE switching stack
    mov ebx, [esp+24]
    mov ecx, [esp+28]

    ; switch page directory if non-zero
    test ecx, ecx
    jz .skip_cr3
    mov cr3, ecx
.skip_cr3:

    ; switch to new stack
    mov esp, ebx

    ; restore registers from new stack
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret