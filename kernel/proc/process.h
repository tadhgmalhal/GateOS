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

typedef struct process
{
    uint32_t         pid;
    char             name[PROCESS_NAME_MAX];
    process_state_t  state;
    uint32_t         esp;
    uint32_t         user_esp;
    uint32_t         user_eip;
    page_dir_t      *page_dir;
    uint32_t         kernel_stack;
    uint32_t         kernel_stack_top;
    uint32_t         priority;
    uint32_t         ticks;
    uint32_t         sleep_until;
    struct process  *next;
    struct process  *parent;
} process_t;

process_t  *process_create(const char *name, uint32_t priority);
process_t  *process_create_user(const char *name, uint32_t priority, void *code, uint32_t code_size);
void        process_destroy(process_t *proc);
process_t  *process_get_current();
process_t  *process_get_list();
void        process_set_current(process_t *proc);
void        process_init();

#endif