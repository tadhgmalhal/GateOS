#include "kprintf.h"
#include "../vga.h"
#include <stdarg.h>

static int cursor_col = 0;
static int cursor_row = 1;

static void kprintf_putchar(char c)
{
    if (c == '\n')
    {
        cursor_col = 0;
        cursor_row++;
    }
    else if (c == '\t')
    {
        cursor_col += 4;
    }
    else
    {
        vga_putchar(c, cursor_col, cursor_row);
        cursor_col++;

        if (cursor_col >= 80)
        {
            cursor_col = 0;
            cursor_row++;
        }
    }

    if (cursor_row >= 25)
    {
        vga_scroll();
        cursor_row = 24;
    }
}

static void kprintf_putstr(const char *str)
{
    int i = 0;
    while (str[i] != '\0')
    {
        kprintf_putchar(str[i]);
        i++;
    }
}

static void kprintf_putuint(unsigned int n, int base)
{
    char buf[32];
    int i = 0;

    if (n == 0)
    {
        kprintf_putchar('0');
        return;
    }

    while (n > 0)
    {
        int rem = n % base;
        buf[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
        n /= base;
    }

    while (i > 0)
    {
        kprintf_putchar(buf[--i]);
    }
}

static void kprintf_putint(int n)
{
    if (n < 0)
    {
        kprintf_putchar('-');
        n = -n;
    }
    kprintf_putuint((unsigned int)n, 10);
}

void kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int i = 0;
    while (fmt[i] != '\0')
    {
        if (fmt[i] == '%')
        {
            i++;
            switch (fmt[i])
            {
                case 'd':
                    kprintf_putint(va_arg(args, int));
                    break;
                case 'u':
                    kprintf_putuint(va_arg(args, unsigned int), 10);
                    break;
                case 'x':
                    kprintf_putstr("0x");
                    kprintf_putuint(va_arg(args, unsigned int), 16);
                    break;
                case 's':
                    kprintf_putstr(va_arg(args, const char *));
                    break;
                case 'c':
                    kprintf_putchar((char)va_arg(args, int));
                    break;
                case '%':
                    kprintf_putchar('%');
                    break;
                default:
                    kprintf_putchar('%');
                    kprintf_putchar(fmt[i]);
                    break;
            }
        }
        else
        {
            kprintf_putchar(fmt[i]);
        }
        i++;
    }

    va_end(args);
}

void kpanic(const char *file, int line, const char *msg)
{
    kprintf("\n*** KERNEL PANIC ***\n");
    kprintf("  file: %s\n", file);
    kprintf("  line: %d\n", line);
    kprintf("  msg:  %s\n", msg);
    kprintf("System halted.");

    __asm__ volatile("cli");
    while (1)
    {
        __asm__ volatile("hlt");
    }
}

int kprintf_get_row()
{
    return cursor_row;
}