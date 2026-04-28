#include "syscall.h"
#include "idt.h"
#include "../proc/process.h"
#include "../proc/scheduler.h"
#include "../mm/kheap.h"
#include "../mm/vmm.h"
#include "../mm/pmm.h"
#include "../lib/kprintf.h"

extern void syscall_entry();

static uint32_t user_heap_end = 0x20000000; // 512MB mark — user heap start

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
    // stub — keyboard input will be wired properly in Phase 4
    // for now return 0 bytes read
    (void)regs;
    regs->eax = 0;
}

static void sys_getpid(registers_t *regs)
{
    regs->eax = process_get_current()->pid;
}

static void sys_waitpid(registers_t *regs)
{
    uint32_t target_pid = regs->ebx;
    process_t *list     = process_get_list();

    while (1)
    {
        process_t *proc = list;
        int found = 0;

        while (proc)
        {
            if (proc->pid == target_pid)
            {
                found = 1;
                if (proc->state == PROCESS_ZOMBIE)
                {
                    // process has exited — return its pid
                    regs->eax = target_pid;
                    return;
                }
                break;
            }
            proc = proc->next;
        }

        if (!found)
        {
            // process doesn't exist
            regs->eax = (uint32_t)-1;
            return;
        }

        // process still running — yield and check again
        scheduler_yield();
    }
}

static void sys_sbrk(registers_t *regs)
{
    int32_t  increment = (int32_t)regs->ebx;
    uint32_t old_end   = user_heap_end;

    if (increment == 0)
    {
        regs->eax = old_end;
        return;
    }

    if (increment > 0)
    {
        uint32_t pages = ((uint32_t)increment + PAGE_SIZE - 1) / PAGE_SIZE;
        for (uint32_t i = 0; i < pages; i++)
        {
            uint32_t phys = pmm_alloc_frame();
            vmm_map(process_get_current()->page_dir,
                    user_heap_end + i * PAGE_SIZE,
                    phys,
                    PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        }
        user_heap_end += pages * PAGE_SIZE;
    }
    else
    {
        uint32_t pages = ((uint32_t)(-increment) + PAGE_SIZE - 1) / PAGE_SIZE;
        for (uint32_t i = 0; i < pages; i++)
        {
            user_heap_end -= PAGE_SIZE;
            vmm_unmap(process_get_current()->page_dir, user_heap_end);
        }
    }

    regs->eax = old_end;
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
        case SYS_WAITPID:
            sys_waitpid(regs);
            break;
        case SYS_SBRK:
            sys_sbrk(regs);
            break;
        default:
            kprintf("Unknown syscall: %d\n", regs->eax);
            regs->eax = (uint32_t)-1;
            break;
    }
}

void syscall_init()
{
    idt_set_gate(0x80, (uint32_t)syscall_entry, 0x08, 0xEE);
    kprintf("Syscall interface initialized.\n");
    kprintf("  int 0x80 gate active.\n");
}