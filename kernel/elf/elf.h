#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include "../proc/process.h"

// ELF magic number
#define ELF_MAGIC 0x464C457F  // 0x7F 'E' 'L' 'F' in little endian

// ELF types
#define ET_EXEC 2  // executable file

// ELF machine
#define EM_386 3   // x86

// Program header types
#define PT_LOAD 1  // loadable segment

// Program header flags
#define PF_X 0x1   // execute
#define PF_W 0x2   // write
#define PF_R 0x4   // read

typedef struct
{
    uint32_t e_magic;       // magic number — must be 0x7F 'E' 'L' 'F'
    uint8_t  e_class;       // 1 = 32-bit, 2 = 64-bit
    uint8_t  e_data;        // 1 = little endian, 2 = big endian
    uint8_t  e_version;     // ELF version — always 1
    uint8_t  e_osabi;       // OS ABI — 0 = System V
    uint8_t  e_padding[8];  // unused padding
    uint16_t e_type;        // file type — ET_EXEC for executable
    uint16_t e_machine;     // architecture — EM_386 for x86
    uint32_t e_version2;    // ELF version again — always 1
    uint32_t e_entry;       // entry point virtual address
    uint32_t e_phoff;       // offset to program header table
    uint32_t e_shoff;       // offset to section header table (unused)
    uint32_t e_flags;       // processor flags (unused on x86)
    uint16_t e_ehsize;      // size of this header — 52 bytes
    uint16_t e_phentsize;   // size of one program header entry — 32 bytes
    uint16_t e_phnum;       // number of program header entries
    uint16_t e_shentsize;   // size of one section header entry
    uint16_t e_shnum;       // number of section header entries
    uint16_t e_shstrndx;    // index of section name string table
} __attribute__((packed)) elf_header_t;

typedef struct
{
    uint32_t p_type;    // segment type — PT_LOAD = load into memory
    uint32_t p_offset;  // offset in file where segment data starts
    uint32_t p_vaddr;   // virtual address to load segment at
    uint32_t p_paddr;   // physical address (ignored, we use paging)
    uint32_t p_filesz;  // size of segment in file
    uint32_t p_memsz;   // size of segment in memory (>= filesz, extra = BSS)
    uint32_t p_flags;   // permissions — PF_R, PF_W, PF_X
    uint32_t p_align;   // alignment — usually 0x1000 (4KB)
} __attribute__((packed)) elf_program_header_t;

typedef enum
{
    ELF_OK,
    ELF_BAD_MAGIC,
    ELF_BAD_ARCH,
    ELF_BAD_TYPE,
    ELF_NO_LOAD_SEGMENTS
} elf_result_t;

elf_result_t elf_load(void *data, uint32_t size, process_t *proc);

#endif