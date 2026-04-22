#include "vga.h"
#include <stdint.h>

#define VGA_ADDRESS    0xB8000
#define VGA_WIDTH      80
#define VGA_HEIGHT     25
#define WHITE_ON_BLACK VGA_COLOR(VGA_WHITE, VGA_BLACK)

static uint16_t *vga = (uint16_t *)VGA_ADDRESS;

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %1, %0" : : "dN"(port), "a"(value));
}

void vga_clear()
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        vga[i] = (uint16_t)(' ') | (uint16_t)(WHITE_ON_BLACK << 8);
    }
}

void vga_print(const char *str, int col, int row)
{
    int i = 0;
    while (str[i] != '\0')
    {
        int index = row * VGA_WIDTH + col + i;
        vga[index] = (uint16_t)(str[i]) | (uint16_t)(WHITE_ON_BLACK << 8);
        i++;
    }
}

void vga_print_color(const char *str, int col, int row, uint8_t color)
{
    int i = 0;
    while (str[i] != '\0')
    {
        int index = row * VGA_WIDTH + col + i;
        vga[index] = (uint16_t)(str[i]) | (uint16_t)(color << 8);
        i++;
    }
}

void vga_putchar(char c, int col, int row)
{
    int index = row * VGA_WIDTH + col;
    vga[index] = (uint16_t)(c) | (uint16_t)(WHITE_ON_BLACK << 8);
}

void vga_putchar_color(char c, int col, int row, uint8_t color)
{
    int index = row * VGA_WIDTH + col;
    vga[index] = (uint16_t)(c) | (uint16_t)(color << 8);
}

void vga_scroll()
{
    for (int row = 1; row < VGA_HEIGHT; row++)
    {
        for (int col = 0; col < VGA_WIDTH; col++)
        {
            vga[(row - 1) * VGA_WIDTH + col] = vga[row * VGA_WIDTH + col];
        }
    }

    for (int col = 0; col < VGA_WIDTH; col++)
    {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = (uint16_t)(' ') | (uint16_t)(WHITE_ON_BLACK << 8);
    }
}

void vga_enable_cursor()
{
    // cursor scanline start 14, end 15 — thin underline at bottom of cell
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x0E);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);
}

void vga_set_cursor(int col, int row)
{
    uint16_t pos = row * VGA_WIDTH + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}