#include "vmm.h"
#include "pmm.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

static page_dir_t *kernel_dir = 0;

static page_table_t *vmm_get_or_create_table(page_dir_t *dir, uint32_t dir_index, uint32_t flags)
{
    if (dir->entries[dir_index] & PAGE_PRESENT)
    {
        return (page_table_t *)PAGE_FRAME_ADDR(dir->entries[dir_index]);
    }

    page_table_t *table = (page_table_t *)pmm_alloc_frame();
    memset(table, 0, sizeof(page_table_t));
    dir->entries[dir_index] = (uint32_t)table | PAGE_PRESENT | PAGE_WRITABLE | flags;
    return table;
}

void vmm_map(page_dir_t *dir, uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t dir_index   = PAGE_DIR_INDEX(virt);
    uint32_t table_index = PAGE_TABLE_INDEX(virt);

    page_table_t *table = vmm_get_or_create_table(dir, dir_index, flags);
    table->entries[table_index] = (phys & 0xFFFFF000) | PAGE_PRESENT | flags;

    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_unmap(page_dir_t *dir, uint32_t virt)
{
    uint32_t dir_index   = PAGE_DIR_INDEX(virt);
    uint32_t table_index = PAGE_TABLE_INDEX(virt);

    if (!(dir->entries[dir_index] & PAGE_PRESENT))
    {
        return;
    }

    page_table_t *table = (page_table_t *)PAGE_FRAME_ADDR(dir->entries[dir_index]);
    table->entries[table_index] = 0;

    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_switch_dir(page_dir_t *dir)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(dir) : "memory");
}

page_dir_t *vmm_get_kernel_dir()
{
    return kernel_dir;
}

page_dir_t *vmm_create_user_dir()
{
    page_dir_t *dir = (page_dir_t *)pmm_alloc_frame();
    memset(dir, 0, sizeof(page_dir_t));

    uint32_t kernel_start_index = PAGE_DIR_INDEX(KERNEL_VIRTUAL_BASE);
    for (uint32_t i = kernel_start_index; i < 1024; i++)
    {
        dir->entries[i] = kernel_dir->entries[i];
    }

    return dir;
}

void vmm_destroy_user_dir(page_dir_t *dir)
{
    uint32_t kernel_start_index = PAGE_DIR_INDEX(KERNEL_VIRTUAL_BASE);

    for (uint32_t i = 0; i < kernel_start_index; i++)
    {
        if (dir->entries[i] & PAGE_PRESENT)
        {
            pmm_free_frame(PAGE_FRAME_ADDR(dir->entries[i]));
        }
    }

    pmm_free_frame((uint32_t)dir);
}

void vmm_init()
{
    kernel_dir = (page_dir_t *)pmm_alloc_frame();
    memset(kernel_dir, 0, sizeof(page_dir_t));

    for (uint32_t addr = 0; addr < 0x800000; addr += PAGE_SIZE)
    {
        vmm_map(kernel_dir, addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }

    vmm_switch_dir(kernel_dir);

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

    kprintf("VMM initialized.\n");
    kprintf("  Paging enabled.\n");
    kprintf("  Kernel identity mapped 0x0 - 0x800000\n");
}