#include "timer.h"
#include "../cpu/irq.h"
#include "../lib/kprintf.h"

static uint32_t tick = 0;

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %1, %0" : : "dN"(port), "a"(value));
}

void scheduler_tick(registers_t *regs);

static void timer_callback(registers_t *regs)
{
    tick++;
    scheduler_tick(regs);
}

void timer_init(uint32_t frequency)
{
    uint32_t divisor = 1193180 / frequency;

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    irq_register(0, timer_callback);
}

uint32_t timer_get_ticks()
{
    return tick;
}