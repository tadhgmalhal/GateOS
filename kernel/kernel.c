#include <stdint.h>
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/irq.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
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
    vga_print("GDT initialized.", 0, 1);
    vga_print("IDT initialized.", 0, 2);
    vga_print("IRQs enabled.", 0, 3);
    vga_print("Timer and keyboard active.", 0, 4);
    vga_print("> ", 0, 6);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}