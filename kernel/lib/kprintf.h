#ifndef KPRINTF_H
#define KPRINTF_H

void kprintf(const char *fmt, ...);
void kpanic(const char *file, int line, const char *msg);
int  kprintf_get_row();

#define PANIC(msg) kpanic(__FILE__, __LINE__, msg)

#endif