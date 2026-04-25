#include "isr.h"
#include "../vga.h"
#include "../lib/kprintf.h"

static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(registers_t *regs)
{
    if (regs->int_no == 14)
    {
        uint32_t faulting_addr;
        __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_addr));

        kprintf("\n*** PAGE FAULT ***\n");
        kprintf("  faulting address: %x\n", faulting_addr);
        kprintf("  error code:       %x\n", regs->err_code);
        kprintf("  present:          %d\n", regs->err_code & 0x1);
        kprintf("  write:            %d\n", (regs->err_code >> 1) & 0x1);
        kprintf("  user mode:        %d\n", (regs->err_code >> 2) & 0x1);
        kprintf("  eip:              %x\n", regs->eip);

        while (1)
        {
            __asm__ volatile("hlt");
        }
    }

    kprintf("\n*** EXCEPTION: %s ***\n", exception_messages[regs->int_no]);
    kprintf("  int_no:   %d\n", regs->int_no);
    kprintf("  err_code: %x\n", regs->err_code);
    kprintf("  eip:      %x\n", regs->eip);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}