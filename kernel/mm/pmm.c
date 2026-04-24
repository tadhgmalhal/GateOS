#include "pmm.h"
#include "../lib/kprintf.h"
#include "../lib/string.h"

#define PMM_BITMAP_SIZE 0x8000

static uint32_t pmm_bitmap[PMM_BITMAP_SIZE / 4];
static uint32_t total_frames = 0;
static uint32_t free_frames = 0;

static void pmm_set_bit(uint32_t frame)
{
    pmm_bitmap[frame / 32] |= (1 << (frame % 32));
}

static void pmm_clear_bit(uint32_t frame)
{
    pmm_bitmap[frame / 32] &= ~(1 << (frame % 32));
}

static int pmm_test_bit(uint32_t frame)
{
    return pmm_bitmap[frame / 32] & (1 << (frame % 32));
}

void pmm_init(multiboot_info_t *mboot)
{
    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));

    multiboot_mmap_entry_t *mmap = (multiboot_mmap_entry_t *)mboot->mmap_addr;
    multiboot_mmap_entry_t *end  = (multiboot_mmap_entry_t *)(mboot->mmap_addr + mboot->mmap_length);

    while (mmap < end)
    {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            // only handle memory within 32-bit range
            if (mmap->addr < 0x100000000ULL)
            {
                uint32_t addr   = (uint32_t)mmap->addr;
                uint32_t length = (uint32_t)mmap->len;

                uint32_t frame      = addr / PAGE_SIZE;
                uint32_t frame_count = length / PAGE_SIZE;

                for (uint32_t i = 0; i < frame_count; i++)
                {
                    pmm_clear_bit(frame + i);
                    free_frames++;
                    total_frames++;
                }
            }
        }

        mmap = (multiboot_mmap_entry_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    pmm_set_bit(0);
    free_frames--;

    kprintf("PMM initialized.\n");
    kprintf("  Total frames: %u\n", total_frames);
    kprintf("  Free frames:  %u\n", free_frames);
    kprintf("  Total RAM:    %u MB\n", (total_frames * PAGE_SIZE) / (1024 * 1024));
}

uint32_t pmm_alloc_frame()
{
    for (uint32_t i = 0; i < total_frames / 32; i++)
    {
        if (pmm_bitmap[i] != 0xFFFFFFFF)
        {
            for (uint32_t j = 0; j < 32; j++)
            {
                uint32_t frame = i * 32 + j;
                if (!pmm_test_bit(frame))
                {
                    pmm_set_bit(frame);
                    free_frames--;
                    return frame * PAGE_SIZE;
                }
            }
        }
    }

    PANIC("pmm_alloc_frame: out of physical memory");
    return 0;
}

void pmm_free_frame(uint32_t addr)
{
    uint32_t frame = addr / PAGE_SIZE;

    if (pmm_test_bit(frame) == 0)
    {
        PANIC("pmm_free_frame: freeing an already free frame");
    }

    pmm_clear_bit(frame);
    free_frames++;
}

uint32_t pmm_get_free_frames()
{
    return free_frames;
}

uint32_t pmm_get_total_frames()
{
    return total_frames;
}