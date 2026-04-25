#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

void  heap_init();
void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *ptr, size_t size);
void  kfree(void *ptr);

#endif