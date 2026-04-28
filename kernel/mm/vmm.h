#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PAGE_PRESENT    0x1
#define PAGE_WRITABLE   0x2
#define PAGE_USER       0x4

#define PAGE_DIR_INDEX(addr)   (((addr) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(addr) (((addr) >> 12) & 0x3FF)
#define PAGE_FRAME_ADDR(entry) ((entry) & 0xFFFFF000)

#define KERNEL_VIRTUAL_BASE 0xC0000000
#define USER_STACK_TOP      0xBFFFF000
#define USER_STACK_SIZE     0x100000
#define USER_CODE_BASE      0x00400000

typedef uint32_t page_dir_entry_t;
typedef uint32_t page_table_entry_t;

typedef struct
{
    page_table_entry_t entries[1024];
} page_table_t;

typedef struct
{
    page_dir_entry_t entries[1024];
} page_dir_t;

void        vmm_init();
void        vmm_map(page_dir_t *dir, uint32_t virt, uint32_t phys, uint32_t flags);
void        vmm_unmap(page_dir_t *dir, uint32_t virt);
void        vmm_switch_dir(page_dir_t *dir);
page_dir_t *vmm_get_kernel_dir();
page_dir_t *vmm_create_user_dir();
void        vmm_destroy_user_dir(page_dir_t *dir);

#endif