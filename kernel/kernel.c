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

    // register devices
    devfs_register("keyboard", keyboard_get_device());

    // mount filesystems
    vfs_mount("/dev", devfs_get_root());
    vfs_mount("/tmp", tmpfs_get_root());

    // tmpfs test
    kprintf("Testing tmpfs...\n");

    vfs_node_t *tmp = vfs_open("/tmp");
    if (!tmp)
    {
        kprintf("  /tmp open: FAIL\n");
    }
    else
    {
        kprintf("  /tmp open: PASS\n");

        // create a file in /tmp
        vfs_node_t *file = tmpfs_create_file(tmp, "test.txt");
        if (!file)
        {
            kprintf("  create file: FAIL\n");
        }
        else
        {
            kprintf("  create file: PASS\n");

            // write to it
            const uint8_t *msg = (const uint8_t *)"Hello from tmpfs!";
            uint32_t written = vfs_write(file, 0, 17, msg);
            kprintf("  write %d bytes: %s\n", written, written == 17 ? "PASS" : "FAIL");

            // read it back
            uint8_t buf[32];
            memset(buf, 0, 32);
            uint32_t read = vfs_read(file, 0, 17, buf);
            kprintf("  read %d bytes: %s\n", read, read == 17 ? "PASS" : "FAIL");
            kprintf("  content: %s\n", (char *)buf);

            // open via VFS path
            vfs_node_t *found = vfs_finddir(tmp, "test.txt");
            if (found)
            {
                kprintf("  finddir: PASS\n");
            }
            else
            {
                kprintf("  finddir: FAIL\n");
            }
        }

        // create a subdirectory
        vfs_node_t *subdir = tmpfs_create_dir(tmp, "subdir");
        if (subdir)
        {
            kprintf("  create dir: PASS\n");
            vfs_node_t *subfile = tmpfs_create_file(subdir, "nested.txt");
            if (subfile)
            {
                kprintf("  nested file: PASS\n");
            }
            else
            {
                kprintf("  nested file: FAIL\n");
            }
        }
        else
        {
            kprintf("  create dir: FAIL\n");
        }

        // readdir
        kprintf("  /tmp contents:\n");
        vfs_dirent_t *dirent;
        uint32_t i = 0;
        while ((dirent = vfs_readdir(tmp, i)) != 0)
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