#include "kheap.h"
#include "vmm.h"
#include "pmm.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

#define HEAP_START    0x01000000  // 16MB mark
#define HEAP_MAX      0x04000000  // 64MB mark
#define HEAP_MIN_SIZE 0x100000    // start with 1MB of heap

// every allocation has this header placed before it
typedef struct block_header
{
    size_t               size;
    int                  is_free;
    struct block_header *next;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

static block_header_t *heap_start_block = 0;
static uint32_t        heap_end         = HEAP_START;

static void heap_expand(size_t size)
{
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (size_t i = 0; i < pages; i++)
    {
        if (heap_end >= HEAP_MAX)
        {
            PANIC("kheap: heap exhausted");
        }

        uint32_t phys = pmm_alloc_frame();
        vmm_map(vmm_get_kernel_dir(), heap_end, phys, PAGE_PRESENT | PAGE_WRITABLE);
        heap_end += PAGE_SIZE;
    }
}

void heap_init()
{
    heap_expand(HEAP_MIN_SIZE);

    heap_start_block = (block_header_t *)HEAP_START;
    heap_start_block->size    = HEAP_MIN_SIZE - HEADER_SIZE;
    heap_start_block->is_free = 1;
    heap_start_block->next    = 0;

    kprintf("Heap initialized.\n");
    kprintf("  Start: %x\n", HEAP_START);
    kprintf("  Initial size: %d KB\n", HEAP_MIN_SIZE / 1024);
}

static void coalesce()
{
    block_header_t *cur = heap_start_block;

    while (cur && cur->next)
    {
        if (cur->is_free && cur->next->is_free)
        {
            cur->size += HEADER_SIZE + cur->next->size;
            cur->next  = cur->next->next;
        }
        else
        {
            cur = cur->next;
        }
    }
}

void *kmalloc(size_t size)
{
    if (size == 0)
    {
        return 0;
    }

    if (size % 4 != 0)
    {
        size += 4 - (size % 4);
    }

    block_header_t *cur = heap_start_block;

    while (cur)
    {
        if (cur->is_free && cur->size >= size)
        {
            if (cur->size >= size + HEADER_SIZE + 4)
            {
                block_header_t *new_block = (block_header_t *)((uint8_t *)cur + HEADER_SIZE + size);
                new_block->size    = cur->size - size - HEADER_SIZE;
                new_block->is_free = 1;
                new_block->next    = cur->next;

                cur->size = size;
                cur->next = new_block;
            }

            cur->is_free = 0;
            return (void *)((uint8_t *)cur + HEADER_SIZE);
        }

        if (!cur->next)
        {
            size_t expand_size = size + HEADER_SIZE;
            heap_expand(expand_size);

            block_header_t *new_block = (block_header_t *)heap_end - 1;
            new_block = (block_header_t *)((uint32_t)heap_end - expand_size);
            new_block->size    = size;
            new_block->is_free = 0;
            new_block->next    = 0;
            cur->next = new_block;
            return (void *)((uint8_t *)new_block + HEADER_SIZE);
        }

        cur = cur->next;
    }

    PANIC("kmalloc: heap corrupted");
    return 0;
}

void kfree(void *ptr)
{
    if (!ptr)
    {
        return;
    }

    block_header_t *header = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    header->is_free = 1;
    coalesce();
}

void *kcalloc(size_t count, size_t size)
{
    size_t total = count * size;
    void *ptr = kmalloc(total);

    if (ptr)
    {
        memset(ptr, 0, total);
    }

    return ptr;
}

void *krealloc(void *ptr, size_t size)
{
    if (!ptr)
    {
        return kmalloc(size);
    }

    if (size == 0)
    {
        kfree(ptr);
        return 0;
    }

    block_header_t *header = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);

    if (header->size >= size)
    {
        return ptr;
    }

    void *new_ptr = kmalloc(size);

    if (new_ptr)
    {
        memcpy(new_ptr, ptr, header->size);
        kfree(ptr);
    }

    return new_ptr;
}