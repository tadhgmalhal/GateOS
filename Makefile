IMAGE_NAME = gateos-toolchain
SRC_DIR = $(shell pwd)

.PHONY: docker-build build run clean

docker-build:
	docker build -t $(IMAGE_NAME) .

build:
	docker run --rm -v $(SRC_DIR):/gateos:z $(IMAGE_NAME) make _build

_build:
	nasm -f elf32 boot/boot.asm -o boot/boot.o
	nasm -f elf32 kernel/cpu/gdt.asm -o kernel/cpu/gdt_asm.o
	nasm -f elf32 kernel/cpu/idt.asm -o kernel/cpu/idt_asm.o
	nasm -f elf32 kernel/cpu/irq.asm -o kernel/cpu/irq_asm.o
	nasm -f elf32 kernel/cpu/syscall.asm -o kernel/cpu/syscall_asm.o
	nasm -f elf32 kernel/proc/context_switch.asm -o kernel/proc/context_switch.o
	nasm -f elf32 kernel/proc/userspace.asm -o kernel/proc/userspace_asm.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/kernel.c -o kernel/kernel.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/cpu/gdt.c -o kernel/cpu/gdt.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/cpu/idt.c -o kernel/cpu/idt.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/cpu/isr.c -o kernel/cpu/isr.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/cpu/pic.c -o kernel/cpu/pic.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/cpu/irq.c -o kernel/cpu/irq.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/cpu/syscall.c -o kernel/cpu/syscall.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/drivers/timer.c -o kernel/drivers/timer.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/drivers/keyboard.c -o kernel/drivers/keyboard.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/lib/kprintf.c -o kernel/lib/kprintf.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/lib/string.c -o kernel/lib/string.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/mm/pmm.c -o kernel/mm/pmm.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/mm/vmm.c -o kernel/mm/vmm.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/mm/kheap.c -o kernel/mm/kheap.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/proc/process.c -o kernel/proc/process.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/proc/scheduler.c -o kernel/proc/scheduler.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/proc/userspace.c -o kernel/proc/userspace.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-Ikernel -c kernel/vga.c -o kernel/vga.o
	i686-elf-gcc -T linker.ld -o gateos.bin \
		-ffreestanding -O2 -nostdlib \
		boot/boot.o \
		kernel/cpu/gdt_asm.o kernel/cpu/gdt.o \
		kernel/cpu/idt_asm.o kernel/cpu/idt.o \
		kernel/cpu/isr.o \
		kernel/cpu/pic.o \
		kernel/cpu/irq_asm.o kernel/cpu/irq.o \
		kernel/cpu/syscall_asm.o kernel/cpu/syscall.o \
		kernel/drivers/timer.o \
		kernel/drivers/keyboard.o \
		kernel/lib/kprintf.o \
		kernel/lib/string.o \
		kernel/mm/pmm.o \
		kernel/mm/vmm.o \
		kernel/mm/kheap.o \
		kernel/proc/process.o \
		kernel/proc/context_switch.o \
		kernel/proc/scheduler.o \
		kernel/proc/userspace.o \
		kernel/proc/userspace_asm.o \
		kernel/vga.o \
		kernel/kernel.o \
		-lgcc
	mkdir -p iso/boot/grub
	cp gateos.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o gateos.iso iso/

run:
	qemu-system-i386 -cdrom gateos.iso

clean:
	rm -f boot/boot.o kernel/kernel.o gateos.bin gateos.iso
	rm -rf iso/boot
	find kernel -name "*.o" -delete