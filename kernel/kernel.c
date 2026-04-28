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

    // ELF loader test
    static uint8_t test_elf[] = {
        // ELF header
        0x7F, 0x45, 0x4C, 0x46,  // magic
        0x01,                     // 32-bit
        0x01,                     // little endian
        0x01,                     // ELF version
        0x00,                     // System V ABI
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,  // padding
        0x02, 0x00,               // ET_EXEC
        0x03, 0x00,               // EM_386
        0x01, 0x00, 0x00, 0x00,  // version
        0x00, 0x00, 0x40, 0x00,  // entry point 0x400000
        0x34, 0x00, 0x00, 0x00,  // phoff = 52
        0x00, 0x00, 0x00, 0x00,  // shoff = 0
        0x00, 0x00, 0x00, 0x00,  // flags
        0x34, 0x00,               // ehsize = 52
        0x20, 0x00,               // phentsize = 32
        0x01, 0x00,               // phnum = 1
        0x28, 0x00,               // shentsize = 40
        0x00, 0x00,               // shnum = 0
        0x00, 0x00,               // shstrndx = 0

        // Program header
        0x01, 0x00, 0x00, 0x00,  // PT_LOAD
        0x54, 0x00, 0x00, 0x00,  // offset = 84
        0x00, 0x00, 0x40, 0x00,  // vaddr = 0x400000
        0x00, 0x00, 0x40, 0x00,  // paddr = 0x400000
        0x0E, 0x00, 0x00, 0x00,  // filesz = 14
        0x0E, 0x00, 0x00, 0x00,  // memsz = 14
        0x05, 0x00, 0x00, 0x00,  // flags = PF_R | PF_X
        0x00, 0x10, 0x00, 0x00,  // align = 0x1000

        // Code
        0xB8, 0x00, 0x00, 0x00, 0x00,  // mov eax, 0 (SYS_EXIT)
        0xBB, 0x00, 0x00, 0x00, 0x00,  // mov ebx, 0 (exit code)
        0xCD, 0x80,                     // int 0x80
        0xEB, 0xFE                      // jmp $
    };

    kprintf("Testing ELF loader...\n");

    process_t *elf_proc = (process_t *)kmalloc(sizeof(process_t));
    memset(elf_proc, 0, sizeof(process_t));
    elf_proc->pid      = 99;
    elf_proc->page_dir = vmm_create_user_dir();
    elf_proc->state    = PROCESS_RUNNING;

    elf_result_t result = elf_load(test_elf, sizeof(test_elf), elf_proc);

    if (result != ELF_OK)
    {
        kprintf("ELF load failed: %d\n", result);
        PANIC("ELF loader test failed");
    }

    kprintf("  ELF loader: PASS\n");
    kprintf("  entry: %x\n", elf_proc->user_eip);
    kprintf("  stack: %x\n", elf_proc->user_esp);

    uint8_t *stack = (uint8_t *)kmalloc(KERNEL_STACK_SIZE);
    elf_proc->kernel_stack     = (uint32_t)stack;
    elf_proc->kernel_stack_top = (uint32_t)(stack + KERNEL_STACK_SIZE);

    process_set_current(elf_proc);
    tss_set_stack(0x10, elf_proc->kernel_stack_top);
    vmm_switch_dir(elf_proc->page_dir);

    jump_to_userspace(elf_proc->user_eip, elf_proc->user_esp);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}