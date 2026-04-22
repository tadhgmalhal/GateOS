#include <stdint.h>
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/irq.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "lib/kprintf.h"
#include "vga.h"

void kernel_main()
{
    gdt_init();
    idt_init();
    irq_init();
    timer_init(100);
    keyboard_init();

    vga_clear();
    vga_print("GateOS v0.1", 0, 0);

    kprintf("GDT initialized.\n");
    kprintf("IDT initialized.\n");
    kprintf("IRQs enabled.\n");
    kprintf("Timer running at %d Hz.\n", 100);
    kprintf("Keyboard active.\n");
    kprintf("kprintf working. Hex test: %x\n", 0xDEADBEEF);

    vga_print("> ", 0, 23);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}