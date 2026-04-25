#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "../boot/multiboot.h"

#define PAGE_SIZE 4096

void pmm_init(multiboot_info_t *mboot);
uint32_t pmm_alloc_frame();
void pmm_free_frame(uint32_t addr);
uint32_t pmm_get_free_frames();
uint32_t pmm_get_total_frames();

#endif