#include <stdint.h>
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/irq.h"
#include "cpu/syscall.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "drivers/ata.h"
#include "drivers/disk_cache.h"
#include "lib/kprintf.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/kheap.h"
#include "proc/process.h"
#include "proc/scheduler.h"
#include "proc/userspace.h"
#include "elf/elf.h"
#include "boot/multiboot.h"
#include "vga.h"

void kernel_main(multiboot_info_t *mboot)
{
    gdt_init();
    idt_init();
    syscall_init();
    irq_init();
    timer_init(100);
    keyboard_init();

    vga_clear();
    vga_enable_cursor();
    vga_print_color("GateOS v0.1", 0, 0, VGA_COLOR(VGA_LIGHT_CYAN, VGA_BLACK));

    kprintf("GDT initialized.\n");
    kprintf("IDT initialized.\n");
    kprintf("IRQs enabled.\n");
    kprintf("Syscalls active.\n");
    kprintf("Timer running at %d Hz.\n", 100);
    kprintf("Keyboard active.\n");

    pmm_init(mboot);
    vmm_init();
    heap_init();
    process_init();
    scheduler_init();
    ata_init();
    disk_cache_init();

    // cache test
    uint8_t write_buf[ATA_SECTOR_SIZE];
    uint8_t read_buf[ATA_SECTOR_SIZE];

    memset(write_buf, 0xAB, ATA_SECTOR_SIZE);

    disk_cache_write(1, write_buf);
    kprintf("Cache write: OK\n");

    memset(read_buf, 0, ATA_SECTOR_SIZE);
    disk_cache_read(1, read_buf);

    int match = 1;
    for (int i = 0; i < ATA_SECTOR_SIZE; i++)
    {
        if (read_buf[i] != 0xAB) { match = 0; break; }
    }
    kprintf("Cache read (hit): %s\n", match ? "OK" : "FAILED");

    disk_cache_flush();
    kprintf("Cache flush: OK\n");

    memset(read_buf, 0, ATA_SECTOR_SIZE);
    disk_cache_read(1, read_buf);
    match = 1;
    for (int i = 0; i < ATA_SECTOR_SIZE; i++)
    {
        if (read_buf[i] != 0xAB) { match = 0; break; }
    }
    kprintf("Cache read (hit 2): %s\n", match ? "OK" : "FAILED");

    kprintf("Cache hits:   %u\n", disk_cache_get_hits());
    kprintf("Cache misses: %u\n", disk_cache_get_misses());

    kprintf("GateOS ready.\n");

    while (1)
    {
        __asm__ volatile("hlt");
    }
}