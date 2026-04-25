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

    proc->pid      = next_pid++;
    proc->state    = PROCESS_READY;
    proc->priority = priority;
    proc->ticks    = 0;
    proc->next     = 0;
    proc->parent   = current_process;
    proc->esp      = 0;

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

void process_destroy(process_t *proc)
{
    proc->state = PROCESS_ZOMBIE;
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