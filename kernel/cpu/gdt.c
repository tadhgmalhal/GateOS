#include "gdt.h"

#define GDT_ENTRIES 6

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;
static tss_entry_t tss;

extern void gdt_flush(uint32_t);
extern void tss_flush();

static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
    gdt[index].base_low    = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;
    gdt[index].limit_low   = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    gdt[index].access      = access;
}

static void tss_init(uint32_t kernel_ss, uint32_t kernel_esp)
{
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = base + sizeof(tss_entry_t);

    gdt_set_entry(5, base, limit, 0xE9, 0x00);

    tss.ss0  = kernel_ss;
    tss.esp0 = kernel_esp;
    tss.cs   = 0x08;
    tss.ss   = 0x10;
    tss.ds   = 0x10;
    tss.es   = 0x10;
    tss.fs   = 0x10;
    tss.gs   = 0x10;
}

void tss_set_stack(uint32_t ss0, uint32_t esp0)
{
    tss.ss0  = ss0;
    tss.esp0 = esp0;
}

void gdt_init()
{
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_set_entry(0, 0, 0x00000000, 0x00, 0x00); // null
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // kernel code
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // kernel data
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // user code
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // user data
    tss_init(0x10, 0x0);

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
}