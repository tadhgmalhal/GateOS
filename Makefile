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
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-c kernel/kernel.c -o kernel/kernel.o
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
		-c kernel/cpu/gdt.c -o kernel/cpu/gdt.o
	i686-elf-gcc -T linker.ld -o gateos.bin \
		-ffreestanding -O2 -nostdlib \
		boot/boot.o kernel/cpu/gdt_asm.o kernel/cpu/gdt.o kernel/kernel.o -lgcc
	mkdir -p iso/boot/grub
	cp gateos.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o gateos.iso iso/

run:
	qemu-system-i386 -cdrom gateos.iso

clean:
	rm -f boot/boot.o kernel/kernel.o gateos.bin gateos.iso
	rm -rf iso/boot