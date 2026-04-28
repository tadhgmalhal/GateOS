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

    // register /dev/keyboard
    devfs_register("keyboard", keyboard_get_device());

    // mount devfs at /dev
    vfs_mount("/dev", devfs_get_root());

    // test VFS
    kprintf("Testing VFS...\n");

    // test /dev/null
    vfs_node_t *null_node = vfs_open("/dev/null");
    if (null_node)
    {
        uint8_t buf[16];
        memset(buf, 0xAA, 16);
        uint32_t written = vfs_write(null_node, 0, 16, buf);
        uint32_t read    = vfs_read(null_node, 0, 16, buf);
        if (written == 16 && read == 0)
        {
            kprintf("  /dev/null: PASS\n");
        }
        else
        {
            kprintf("  /dev/null: FAIL\n");
        }
        vfs_close(null_node);
    }
    else
    {
        kprintf("  /dev/null: FAIL (open returned null)\n");
    }

    // test /dev/zero
    vfs_node_t *zero_node = vfs_open("/dev/zero");
    if (zero_node)
    {
        uint8_t buf[16];
        memset(buf, 0xAA, 16);
        uint32_t read = vfs_read(zero_node, 0, 16, buf);
        int zeroed = 1;
        for (int i = 0; i < 16; i++)
        {
            if (buf[i] != 0) { zeroed = 0; break; }
        }
        if (read == 16 && zeroed)
        {
            kprintf("  /dev/zero: PASS\n");
        }
        else
        {
            kprintf("  /dev/zero: FAIL\n");
        }
        vfs_close(zero_node);
    }
    else
    {
        kprintf("  /dev/zero: FAIL (open returned null)\n");
    }

    // test /dev/keyboard exists
    vfs_node_t *kb_node = vfs_open("/dev/keyboard");
    if (kb_node)
    {
        kprintf("  /dev/keyboard: PASS\n");
        vfs_close(kb_node);
    }
    else
    {
        kprintf("  /dev/keyboard: FAIL\n");
    }

    // test readdir on /dev
    vfs_node_t *dev_node = vfs_open("/dev");
    if (dev_node)
    {
        kprintf("  /dev contents:\n");
        vfs_dirent_t *dirent;
        uint32_t i = 0;
        while ((dirent = vfs_readdir(dev_node, i)) != 0)
        {
            kprintf("    [%d] %s\n", i, dirent->name);
            i++;
        }
    }

    kprintf("GateOS ready.\n");
    vga_print("> ", 0, 23);
    vga_set_cursor(2, 23);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}