#include "userspace.h"

void jump_to_userspace(uint32_t eip, uint32_t esp)
{
    // load user data segment into all data registers
    // 0x23 = GDT entry 4 (user data) with RPL bits set to 3
    __asm__ volatile(
        "mov $0x23, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"

        // build the iret frame on the stack
        "push $0x23\n"      // ss    — user stack segment
        "push %1\n"         // esp   — user stack pointer
        "pushf\n"           // eflags — current flags
        "pop %%eax\n"
        "or $0x200, %%eax\n" // set the interrupt enable flag (IF)
        "push %%eax\n"       // push modified eflags back
        "push $0x1B\n"      // cs    — user code segment
        "push %0\n"         // eip   — user entry point

        "iret\n"            // fire — CPU drops to ring 3 and jumps to eip
        :
        : "r"(eip), "r"(esp)
        : "eax"
    );
}