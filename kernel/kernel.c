#include <stdint.h>
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/irq.h"
#include "cpu/syscall.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "lib/kprintf.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/kheap.h"
#include "proc/process.h"
#include "proc/scheduler.h"
#include "proc/userspace.h"
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
    kprintf("Timer running at %d Hz.\n", 100);
    kprintf("Keyboard active.\n");

    pmm_init(mboot);
    vmm_init();
    heap_init();

    // VMM user dir test
    kprintf("Testing vmm_create_user_dir...\n");

    page_dir_t *test_dir = vmm_create_user_dir();

    if (test_dir == 0)
    {
        PANIC("vmm_create_user_dir returned null");
    }

    int kernel_entries_ok = 1;
    for (int i = 0; i < 1024; i++)
    {
        if (vmm_get_kernel_dir()->entries[i] & PAGE_PRESENT)
        {
            if (test_dir->entries[i] != vmm_get_kernel_dir()->entries[i])
            {
                kernel_entries_ok = 0;
                break;
            }
        }
    }

    if (!kernel_entries_ok)
    {
        PANIC("vmm_create_user_dir: kernel entries not copied correctly");
    }

    vmm_destroy_user_dir(test_dir);
    kprintf("  vmm_create_user_dir: PASS\n");
    kprintf("  vmm_destroy_user_dir: PASS\n");
    kprintf("  Kernel entries copied: PASS\n");

    // process_create_user test
    kprintf("Testing process_create_user...\n");

    uint8_t fake_code[16] = {
        0x90, 0x90, 0x90, 0x90,
        0xEB, 0xFE,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00
    };

    process_t *test_user = process_create_user("test_user", 1, fake_code, sizeof(fake_code));

    if (test_user == 0)
    {
        PANIC("process_create_user returned null");
    }

    if (test_user->page_dir == vmm_get_kernel_dir())
    {
        PANIC("process_create_user: process is using kernel page dir");
    }

    if (test_user->user_eip != USER_CODE_BASE)
    {
        PANIC("process_create_user: wrong user_eip");
    }

    if (test_user->user_esp != USER_STACK_TOP)
    {
        PANIC("process_create_user: wrong user_esp");
    }

    kprintf("  process_create_user: PASS\n");
    kprintf("  user_eip: %x\n", test_user->user_eip);
    kprintf("  user_esp: %x\n", test_user->user_esp);
    kprintf("  page_dir: %x\n", test_user->page_dir);

    process_init();
    scheduler_init();

    kprintf("Step 1: creating user process...\n");

    uint8_t user_code[] = {
        0xEB, 0xFE
    };

    process_t *ring3_proc = process_create_user("ring3_test", 1,
                             user_code, sizeof(user_code));

    kprintf("Step 2: process created, pid=%d\n", ring3_proc->pid);

    tss_set_stack(0x10, ring3_proc->kernel_stack_top);
    kprintf("Step 3: TSS updated.\n");

    vmm_switch_dir(ring3_proc->page_dir);
    kprintf("Step 4: page dir switched.\n");

    kprintf("Step 5: jumping to ring 3...\n");


    kprintf("eip value: %x\n", ring3_proc->user_eip);
    kprintf("esp value: %x\n", ring3_proc->user_esp);
    kprintf("USER_CODE_BASE: %x\n", USER_CODE_BASE);
    kprintf("USER_STACK_TOP: %x\n", USER_STACK_TOP);


    jump_to_userspace(ring3_proc->user_eip, ring3_proc->user_esp);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}