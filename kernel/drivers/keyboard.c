#include "keyboard.h"
#include "../cpu/irq.h"
#include "../vga.h"
#include "../mm/kheap.h"
#include "../lib/string.h"

static int cursor_col = 2;
static int cursor_row = 23;

// keyboard ring buffer
#define KB_BUFFER_SIZE 256
static uint8_t kb_buffer[KB_BUFFER_SIZE];
static uint32_t kb_read_pos  = 0;
static uint32_t kb_write_pos = 0;

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

static void kb_buffer_push(uint8_t c)
{
    uint32_t next = (kb_write_pos + 1) % KB_BUFFER_SIZE;
    if (next != kb_read_pos)
    {
        kb_buffer[kb_write_pos] = c;
        kb_write_pos = next;
    }
}

static int kb_buffer_pop(uint8_t *c)
{
    if (kb_read_pos == kb_write_pos)
    {
        return 0;
    }
    *c = kb_buffer[kb_read_pos];
    kb_read_pos = (kb_read_pos + 1) % KB_BUFFER_SIZE;
    return 1;
}

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
            kb_buffer_push('\n');
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
            kb_buffer_push(c);
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

// VFS read function for /dev/keyboard
static uint32_t keyboard_vfs_read(vfs_node_t *node, uint32_t offset,
                                   uint32_t size, uint8_t *buffer)
{
    (void)node; (void)offset;

    uint32_t bytes_read = 0;
    while (bytes_read < size)
    {
        uint8_t c;
        if (kb_buffer_pop(&c))
        {
            buffer[bytes_read++] = c;
        }
        else
        {
            break;
        }
    }
    return bytes_read;
}

static vfs_node_t keyboard_device;

vfs_node_t *keyboard_get_device()
{
    return &keyboard_device;
}

void keyboard_init()
{
    kb_read_pos  = 0;
    kb_write_pos = 0;

    memset(&keyboard_device, 0, sizeof(vfs_node_t));
    strncpy(keyboard_device.name, "keyboard", VFS_NAME_MAX - 1);
    keyboard_device.type = VFS_CHARDEVICE;
    keyboard_device.read = keyboard_vfs_read;

    irq_register(1, keyboard_callback);
}

void keyboard_set_cursor(int col, int row)
{
    cursor_col = col;
    cursor_row = row;
}