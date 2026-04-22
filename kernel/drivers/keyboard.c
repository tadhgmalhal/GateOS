#include "keyboard.h"
#include "../cpu/irq.h"
#include "../vga.h"

static int cursor_col = 2;
static int cursor_row = 23;

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

            if (cursor_row >= 25)
            {
                vga_scroll();
                cursor_row = 24;
            }

            vga_print("> ", 0, cursor_row);
            cursor_col = 2;
        }
        else if (c == '\b')
        {
            if (cursor_col > 2)
            {
                cursor_col--;
                vga_putchar(' ', cursor_col, cursor_row);
            }
        }
        else if (c != 0)
        {
            vga_putchar(c, cursor_col, cursor_row);
            cursor_col++;

            if (cursor_col >= 80)
            {
                cursor_col = 2;
                cursor_row++;

                if (cursor_row >= 25)
                {
                    vga_scroll();
                    cursor_row = 24;
                }

                vga_print("> ", 0, cursor_row);
            }
        }

        vga_set_cursor(cursor_col, cursor_row);
    }
}

void keyboard_init()
{
    irq_register(1, keyboard_callback);
}