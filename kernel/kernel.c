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
#include "fs/vfs.h"
#include "fs/devfs.h"
#include "fs/tmpfs.h"
#include "fs/ext2.h"
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
    vfs_init();
    devfs_init();
    tmpfs_init();

    // register devices and mount filesystems
    devfs_register("keyboard", keyboard_get_device());
    vfs_mount("/dev", devfs_get_root());
    vfs_mount("/tmp", tmpfs_get_root());

    // mount ext2 at root
    kprintf("Mounting ext2...\n");
    vfs_node_t *ext2_root = ext2_init();
    if (ext2_root)
    {
        vfs_mount("/", ext2_root);

        // test ext2 — list root directory
        kprintf("Root directory contents:\n");
        vfs_dirent_t *dirent;
        uint32_t i = 0;
        while ((dirent = vfs_readdir(ext2_root, i)) != 0)
        {
            kprintf("  [%d] %s\n", i, dirent->name);
            i++;
        }
    }
    else
    {
        kprintf("ext2: mount failed\n");
    }

    kprintf("GateOS ready.\n");
    vga_print("> ", 0, 23);
    vga_set_cursor(2, 23);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}