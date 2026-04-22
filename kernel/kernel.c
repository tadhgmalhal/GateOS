#include <stdint.h>
#include "cpu/gdt.h"

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define WHITE_ON_BLACK 0x0F

static uint16_t *vga = (uint16_t *)VGA_ADDRESS;

static void vga_clear()
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        vga[i] = (uint16_t)(' ') | (uint16_t)(WHITE_ON_BLACK << 8);
    }
}

static void vga_print(const char *str, int col, int row)
{
    int i = 0;
    while (str[i] != '\0')
    {
        int index = row * VGA_WIDTH + col + i;
        vga[index] = (uint16_t)(str[i]) | (uint16_t)(WHITE_ON_BLACK << 8);
        i++;
    }
}

void kernel_main()
{
    gdt_init();

    vga_clear();
    vga_print("GateOS v0.1", 0, 0);
    vga_print("Kernel loaded successfully.", 0, 1);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}