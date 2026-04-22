#ifndef VGA_H
#define VGA_H

#include <stdint.h>

// Colors
#define VGA_BLACK         0x0
#define VGA_BLUE          0x1
#define VGA_GREEN         0x2
#define VGA_CYAN          0x3
#define VGA_RED           0x4
#define VGA_MAGENTA       0x5
#define VGA_BROWN         0x6
#define VGA_LIGHT_GREY    0x7
#define VGA_DARK_GREY     0x8
#define VGA_LIGHT_BLUE    0x9
#define VGA_LIGHT_GREEN   0xA
#define VGA_LIGHT_CYAN    0xB
#define VGA_LIGHT_RED     0xC
#define VGA_LIGHT_MAGENTA 0xD
#define VGA_YELLOW        0xE
#define VGA_WHITE         0xF

#define VGA_COLOR(fg, bg) ((bg << 4) | fg)

void vga_clear();
void vga_print(const char *str, int col, int row);
void vga_print_color(const char *str, int col, int row, uint8_t color);
void vga_putchar(char c, int col, int row);
void vga_putchar_color(char c, int col, int row, uint8_t color);
void vga_scroll();
void vga_set_cursor(int col, int row);
void vga_enable_cursor();

#endif