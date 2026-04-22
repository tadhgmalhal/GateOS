#include "pic.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %1, %0" : : "dN"(port), "a"(value));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void io_wait()
{
    outb(0x80, 0x00);
}

void pic_init()
{
    // save masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // ICW1: start initialization sequence
    outb(PIC1_COMMAND, 0x11); io_wait();
    outb(PIC2_COMMAND, 0x11); io_wait();

    // ICW2: remap IRQ vectors
    outb(PIC1_DATA, 0x20); io_wait(); // master IRQs start at vector 32
    outb(PIC2_DATA, 0x28); io_wait(); // slave IRQs start at vector 40

    // ICW3: tell PICs about each other
    outb(PIC1_DATA, 0x04); io_wait(); // master: slave on IRQ2
    outb(PIC2_DATA, 0x02); io_wait(); // slave: cascade identity

    // ICW4: set 8086 mode
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    // restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}