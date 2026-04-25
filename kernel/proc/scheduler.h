#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "../cpu/isr.h"

void     scheduler_init();
void     scheduler_tick(registers_t *regs);
void     scheduler_yield();
void     process_sleep(uint32_t ticks);
uint32_t scheduler_do_switch(uint32_t current_esp);

#endif