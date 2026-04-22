#ifndef VGA_H
#define VGA_H

void vga_clear();
void vga_print(const char *str, int col, int row);
void vga_putchar(char c, int col, int row);
void vga_scroll();

#endif