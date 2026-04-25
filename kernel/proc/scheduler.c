#include "scheduler.h"
#include "process.h"
#include "../cpu/irq.h"
#include "../cpu/gdt.h"
#include "../lib/kprintf.h"

#define SCHEDULER_TICKS 10

static int       switch_pending = 0;
static process_t *next_proc     = 0;
static process_t *old_proc      = 0;

static void schedule()
{
    process_t *current = process_get_current();
    process_t *list    = process_get_list();

    if (!list)
    {
        return;
    }

    process_t *next = current->next;

    while (1)
    {
        if (!next)
        {
            next = list;
        }

        if (next == current)
        {
            return;
        }

        if (next->state == PROCESS_READY)
        {
            break;
        }

        next = next->next;
    }

    if (current->state == PROCESS_RUNNING)
    {
        current->state = PROCESS_READY;
    }

    next->state    = PROCESS_RUNNING;
    next->ticks    = 0;
    old_proc       = current;
    next_proc      = next;
    switch_pending = 1;

    tss_set_stack(0x10, next->kernel_stack_top);
    process_set_current(next);
}

uint32_t scheduler_do_switch(uint32_t current_esp)
{
    if (!switch_pending)
    {
        return 0;
    }

    switch_pending  = 0;
    old_proc->esp   = current_esp;
    return next_proc->esp;
}

void scheduler_tick(registers_t *regs)
{
    (void)regs;

    process_t *current = process_get_current();

    if (!current)
    {
        return;
    }

    current->ticks++;

    if (current->ticks >= SCHEDULER_TICKS)
    {
        current->ticks = 0;
        schedule();
    }
}

void scheduler_yield()
{
    schedule();
}

void scheduler_init()
{
    kprintf("Scheduler initialized.\n");
    kprintf("  Time slice: %d ticks (100ms).\n", SCHEDULER_TICKS);
}