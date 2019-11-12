KERNELOBJS=\
	boot/entry.o \
	boot/long64.o \
	irq/idt.o \
	irq/irq_handler.o \
	irq/isr.o \
	irq/pic.o \
	kernel/kmain.o \
	kernel/vga.o \
	kernel/fb.o \
	mm/mm.o \
	mm/paging.o \
	mm/physman.o \
	utils/cmdline.o \
	utils/ctype.o \
	utils/port.o \
	utils/print.o \
	utils/string.o \
	utils/spinlock.o \
	utils/vsnprintf.o

# Filename of the final kernel binary.
KERNELBIN=wiv.elf

CC=x86_64-elf-gcc
CFLAGS:=-ffreestanding -Wall -Wextra -mno-red-zone -mcmodel=kernel -fno-omit-frame-pointer -g
CPPFLAGS:=-Iinclude -Ilai/include
LDFLAGS:=$(LDFLAGS)
LIBS:=-nostdlib -lgcc -mcmodel=kernel -z max-page-size=0x1000 -fno-omit-frame-pointer
ASM=nasm -g

.PHONY: all kernel clean
.SUFFIXES: .o .c .S .asm

TRASH=$(KERNELOBJS) $(wildcard *.o */*.o */*/*.o) $(KERNELOBJS:.o=.d) $(wildcard *.d */*.d */*/*.d)

all: kernel

kernel: $(KERNELOBJS) linker.ld
	@echo "[LD]  " $< " -> " $(KERNELBIN)
	@$(CC) -T linker.ld -o $(KERNELBIN) $(CFLAGS) $(LIBS) $(KERNELOBJS)
	@grub-file --is-x86-multiboot $(KERNELBIN)

.asm.o:
	@echo "[AS]  " $< " -> " $@
	@nasm -f elf64 $< -o $@

.c.o:
	@echo "[CC]  " $< " -> " $@
	@$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

clean:
	@rm -f $(TRASH) $(KERNELBIN)