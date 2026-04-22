#include "vga.h"
#include <stdint.h>

#define VGA_ADDRESS    0xB8000
#define VGA_WIDTH      80
#define VGA_HEIGHT     25
#define WHITE_ON_BLACK 0x0F

static uint16_t *vga = (uint16_t *)VGA_ADDRESS;

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

void vga_putchar(char c, int col, int row)
{
    int index = row * VGA_WIDTH + col;
    vga[index] = (uint16_t)(c) | (uint16_t)(WHITE_ON_BLACK << 8);
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