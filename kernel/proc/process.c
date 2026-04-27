#include "process.h"
#include "../mm/kheap.h"
#include "../mm/vmm.h"
#include "../mm/pmm.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static process_t *process_list    = 0;
static process_t *current_process = 0;
static uint32_t   next_pid        = 0;

process_t *process_create(const char *name, uint32_t priority)
{
    process_t *proc = (process_t *)kmalloc(sizeof(process_t));

    if (!proc)
    {
        PANIC("process_create: kmalloc failed");
    }

    proc->pid         = next_pid++;
    proc->state       = PROCESS_READY;
    proc->priority    = priority;
    proc->ticks       = 0;
    proc->sleep_until = 0;
    proc->next        = 0;
    proc->parent      = current_process;
    proc->esp         = 0;
    proc->user_esp    = 0;
    proc->user_eip    = 0;

    strncpy(proc->name, name, PROCESS_NAME_MAX - 1);
    proc->name[PROCESS_NAME_MAX - 1] = '\0';

    uint8_t *stack = (uint8_t *)kmalloc(KERNEL_STACK_SIZE);
    if (!stack)
    {
        PANIC("process_create: kernel stack allocation failed");
    }

    proc->kernel_stack     = (uint32_t)stack;
    proc->kernel_stack_top = (uint32_t)(stack + KERNEL_STACK_SIZE);
    proc->page_dir         = vmm_get_kernel_dir();

    if (!process_list)
    {
        process_list = proc;
    }
    else
    {
        process_t *cur = process_list;
        while (cur->next)
        {
            cur = cur->next;
        }
        cur->next = proc;
    }

    return proc;
}

process_t *process_create_user(const char *name, uint32_t priority, void *code, uint32_t code_size)
{
    process_t *proc = process_create(name, priority);

    // give this process its own page directory
    proc->page_dir = vmm_create_user_dir();

    // map and copy code at USER_CODE_BASE
    uint32_t pages_needed = (code_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i < pages_needed; i++)
    {
        uint32_t phys = pmm_alloc_frame();
        vmm_map(proc->page_dir, USER_CODE_BASE + i * PAGE_SIZE, phys,
                PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

        // temporarily map into kernel space so we can write the code
        vmm_map(vmm_get_kernel_dir(), 0xD0000000 + i * PAGE_SIZE, phys,
                PAGE_PRESENT | PAGE_WRITABLE);
    }

    // copy code into the temp kernel mapping
    memcpy((void *)0xD0000000, code, code_size);

    // unmap the temporary kernel mapping
    for (uint32_t i = 0; i < pages_needed; i++)
    {
        vmm_unmap(vmm_get_kernel_dir(), 0xD0000000 + i * PAGE_SIZE);
    }

    // map user stack — 1MB below kernel boundary
    for (uint32_t addr = USER_STACK_TOP - USER_STACK_SIZE;
        addr < USER_STACK_TOP; addr += PAGE_SIZE)
    {
        uint32_t phys = pmm_alloc_frame();
        vmm_map(proc->page_dir, addr, phys,
                PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    }

    proc->user_eip = USER_CODE_BASE;
    proc->user_esp = USER_STACK_TOP;

    return proc;
}

void process_destroy(process_t *proc)
{
    proc->state = PROCESS_ZOMBIE;

    if (proc->page_dir != vmm_get_kernel_dir())
    {
        vmm_destroy_user_dir(proc->page_dir);
    }
}

process_t *process_get_current()
{
    return current_process;
}

process_t *process_get_list()
{
    return process_list;
}

void process_set_current(process_t *proc)
{
    current_process = proc;
}

void process_init()
{
    process_t *idle = process_create("idle", 0);
    idle->state     = PROCESS_RUNNING;
    current_process = idle;
    kprintf("Process manager initialized.\n");
    kprintf("  Idle process created (PID 0).\n");
}