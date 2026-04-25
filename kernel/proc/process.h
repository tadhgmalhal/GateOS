#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "../mm/vmm.h"

#define PROCESS_NAME_MAX  32
#define KERNEL_STACK_SIZE 8192

typedef enum
{
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_ZOMBIE
} process_state_t;

typedef struct
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, ss;
} cpu_state_t;

typedef struct process
{
    uint32_t         pid;
    char             name[PROCESS_NAME_MAX];
    process_state_t  state;
    cpu_state_t      cpu_state;
    page_dir_t      *page_dir;
    uint32_t         kernel_stack;
    uint32_t         kernel_stack_top;
    uint32_t         priority;
    uint32_t         ticks;
    struct process  *next;
    struct process  *parent;
} process_t;

#endif