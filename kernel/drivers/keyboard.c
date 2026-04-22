#include "keyboard.h"
#include "../cpu/irq.h"
#include "../vga.h"

static int cursor_col = 0;
static int cursor_row = 6;

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static const char scancode_map[128] =
{
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,  ' '
};

static void keyboard_callback(registers_t *regs)
{
    (void)regs;

    uint8_t scancode = inb(0x60);

    // ignore key release events (bit 7 set)
    if (scancode & 0x80)
    {
        return;
    }

    if (scancode < 128)
    {
        char c = scancode_map[scancode];

        if (c == '\n')
        {
            cursor_row++;
            cursor_col = 0;
        }
        else if (c == '\b')
        {
            if (cursor_col > 0)
            {
                cursor_col--;
                vga_putchar(' ', cursor_col, cursor_row);
            }
        }
        else if (c != 0)
        {
            vga_putchar(c, cursor_col, cursor_row);
            cursor_col++;
        }
    }
}

void keyboard_init()
{
    irq_register(1, keyboard_callback);
}