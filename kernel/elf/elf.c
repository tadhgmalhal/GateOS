#include "elf.h"
#include "../mm/vmm.h"
#include "../mm/pmm.h"
#include "../lib/string.h"
#include "../lib/kprintf.h"

static void elf_map_segment(process_t *proc, void *data, elf_program_header_t *ph)
{
    uint32_t vaddr   = ph->p_vaddr;
    uint32_t filesz  = ph->p_filesz;
    uint32_t memsz   = ph->p_memsz;
    uint32_t offset  = ph->p_offset;

    // calculate page aligned start and end
    uint32_t vaddr_start = vaddr & 0xFFFFF000;
    uint32_t vaddr_end   = (vaddr + memsz + PAGE_SIZE - 1) & 0xFFFFF000;

    // build VMM flags
    uint32_t flags = PAGE_PRESENT | PAGE_USER;
    if (ph->p_flags & PF_W)
    {
        flags |= PAGE_WRITABLE;
    }

    // allocate and map pages for this segment
    for (uint32_t addr = vaddr_start; addr < vaddr_end; addr += PAGE_SIZE)
    {
        uint32_t phys = pmm_alloc_frame();

        // temporarily map into kernel space so we can write to it
        vmm_map(vmm_get_kernel_dir(), 0xD0000000, phys, PAGE_PRESENT | PAGE_WRITABLE);

        // zero the page first
        memset((void *)0xD0000000, 0, PAGE_SIZE);

        // calculate how much file data falls within this page
        uint32_t page_start = addr;
        uint32_t page_end   = addr + PAGE_SIZE;

        uint32_t file_start = vaddr;
        uint32_t file_end   = vaddr + filesz;

        // find the overlap between this page and the file data
        uint32_t copy_start = (page_start > file_start) ? page_start : file_start;
        uint32_t copy_end   = (page_end   < file_end)   ? page_end   : file_end;

        if (copy_start < copy_end)
        {
            uint32_t page_offset = copy_start - page_start;
            uint32_t file_offset = offset + (copy_start - file_start);
            uint32_t copy_size   = copy_end - copy_start;

            memcpy((void *)(0xD0000000 + page_offset),
                   (uint8_t *)data + file_offset,
                   copy_size);
        }

        // unmap temp kernel mapping
        vmm_unmap(vmm_get_kernel_dir(), 0xD0000000);

        // map into user process at correct virtual address
        vmm_map(proc->page_dir, addr, phys, flags);
    }
}

elf_result_t elf_load(void *data, uint32_t size, process_t *proc)
{
    (void)size;

    elf_header_t *header = (elf_header_t *)data;

    // verify magic number
    if (header->e_magic != ELF_MAGIC)
    {
        kprintf("ELF: bad magic number\n");
        return ELF_BAD_MAGIC;
    }

    // verify 32-bit x86 executable
    if (header->e_class != 1 || header->e_machine != EM_386)
    {
        kprintf("ELF: not a 32-bit x86 binary\n");
        return ELF_BAD_ARCH;
    }

    if (header->e_type != ET_EXEC)
    {
        kprintf("ELF: not an executable\n");
        return ELF_BAD_TYPE;
    }

    kprintf("ELF: entry point %x\n", header->e_entry);
    kprintf("ELF: %d program headers\n", header->e_phnum);

    int loaded = 0;

    // iterate through program headers
    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        elf_program_header_t *ph = (elf_program_header_t *)(
            (uint8_t *)data + header->e_phoff + i * header->e_phentsize
        );

        if (ph->p_type != PT_LOAD)
        {
            continue;
        }

        kprintf("ELF: loading segment %d at %x (%d bytes)\n",
                i, ph->p_vaddr, ph->p_memsz);

        elf_map_segment(proc, data, ph);
        loaded++;
    }

    if (loaded == 0)
    {
        kprintf("ELF: no loadable segments\n");
        return ELF_NO_LOAD_SEGMENTS;
    }

    // set process entry point to ELF entry
    proc->user_eip = header->e_entry;

    // map user stack
    for (uint32_t addr = USER_STACK_TOP - USER_STACK_SIZE;
         addr < USER_STACK_TOP; addr += PAGE_SIZE)
    {
        uint32_t phys = pmm_alloc_frame();
        vmm_map(proc->page_dir, addr, phys,
                PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
    }

    proc->user_esp = USER_STACK_TOP;

    kprintf("ELF: loaded successfully, entry=%x\n", proc->user_eip);
    return ELF_OK;
}