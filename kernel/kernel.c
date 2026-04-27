#include <stdint.h>
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/irq.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "lib/kprintf.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/kheap.h"
#include "proc/process.h"
#include "proc/scheduler.h"
#include "boot/multiboot.h"
#include "vga.h"

void kernel_main(multiboot_info_t *mboot)
{
    gdt_init();
    idt_init();
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

    // verify kernel upper half is copied
    uint32_t kernel_start = PAGE_DIR_INDEX(KERNEL_VIRTUAL_BASE);
    int upper_half_ok = 1;

    for (uint32_t i = kernel_start; i < 1024; i++)
    {
        if (test_dir->entries[i] != vmm_get_kernel_dir()->entries[i])
        {
            upper_half_ok = 0;
            break;
        }
    }

    if (!upper_half_ok)
    {
        PANIC("vmm_create_user_dir: upper half mismatch");
    }

    // verify lower half is empty
    int lower_half_ok = 1;
    for (uint32_t i = 0; i < kernel_start; i++)
    {
        if (test_dir->entries[i] != 0)
        {
            lower_half_ok = 0;
            break;
        }
    }

    if (!lower_half_ok)
    {
        PANIC("vmm_create_user_dir: lower half not zeroed");
    }

    // test destroy
    vmm_destroy_user_dir(test_dir);




    // process_create_user test
    kprintf("Testing process_create_user...\n");

    // a tiny fake program — just some recognizable bytes
    uint8_t fake_code[16] = {
        0x90, 0x90, 0x90, 0x90,  // NOP NOP NOP NOP
        0xEB, 0xFE,              // JMP $ (infinite loop)
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

    kprintf("  vmm_create_user_dir: PASS\n");
    kprintf("  vmm_destroy_user_dir: PASS\n");
    kprintf("  Upper half mapping: PASS\n");
    kprintf("  Lower half zeroed: PASS\n");

    process_init();
    scheduler_init();

    vga_print("> ", 0, 23);
    vga_set_cursor(2, 23);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}