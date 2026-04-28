#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "isr.h"

#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_GETPID  3
#define SYS_WAITPID 4
#define SYS_SBRK    5

void syscall_init();
void syscall_handler(registers_t *regs);

#endif