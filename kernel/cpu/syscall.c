#include "syscall.h"
#include "idt.h"
#include "../proc/process.h"
#include "../proc/scheduler.h"
#include "../lib/kprintf.h"

extern void syscall_entry();

static void sys_exit(registers_t *regs)
{
    process_t *current = process_get_current();
    kprintf("Process %d (%s) exited with code %d\n",
            current->pid, current->name, regs->ebx);
    current->state = PROCESS_ZOMBIE;
    scheduler_yield();
}

static void sys_write(registers_t *regs)
{
    char    *buf = (char *)regs->ecx;
    uint32_t len = regs->edx;

    for (uint32_t i = 0; i < len; i++)
    {
        kprintf("%c", buf[i]);
    }

    regs->eax = len;
}

static void sys_read(registers_t *regs)
{
    // stub — will be implemented properly in Phase 4 with VFS
    regs->eax = 0;
}

static void sys_getpid(registers_t *regs)
{
    regs->eax = process_get_current()->pid;
}

void syscall_handler(registers_t *regs)
{
    switch (regs->eax)
    {
        case SYS_EXIT:
            sys_exit(regs);
            break;
        case SYS_WRITE:
            sys_write(regs);
            break;
        case SYS_READ:
            sys_read(regs);
            break;
        case SYS_GETPID:
            sys_getpid(regs);
            break;
        default:
            kprintf("Unknown syscall: %d\n", regs->eax);
            regs->eax = (uint32_t)-1;
            break;
    }
}

void syscall_init()
{
    // register syscall gate in IDT slot 0x80
    // 0xEE = present, ring 3 accessible, 32-bit interrupt gate
    idt_set_gate(0x80, (uint32_t)syscall_entry, 0x08, 0xEE);
    kprintf("Syscall interface initialized.\n");
    kprintf("  int 0x80 gate active.\n");
}