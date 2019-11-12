#include <stdint.h>
#include <irq/idt.h>
#include <irq/isr.h>
#include <irq/pic.h>
#include <kernel/vga.h>
#include <kernel/kernel.h>
#include <kernel/fb.h>
#include <mm/physman.h>
#include <mm/paging.h>
#include <mm/mm.h>
#include <utils/cmdline.h>
#include <utils/port.h>
#include <utils/print.h>
#include <utils/spinlock.h>
#include <multiboot.h>

void kernel_main(multiboot_info_t *mboot, uint32_t magic) {
    (void)magic;

    vga_init();

    mboot = (multiboot_info_t *)((uintptr_t)mboot + VIRT_PHYS_BASE);
    
    outb(0x3F8 + 1, 0x00);
	outb(0x3F8 + 3, 0x80);
	outb(0x3F8 + 0, 0x03);
	outb(0x3F8 + 1, 0x00);
	outb(0x3F8 + 3, 0x03);
	outb(0x3F8 + 2, 0xC7);
	outb(0x3F8 + 4, 0x0B);

    idt_init();
	pic_remap(0x20, 0x28);

    uintptr_t ptr = mboot->mmap_addr + VIRT_PHYS_BASE;

    physman_init((multiboot_memory_map_t *)ptr, mboot->mmap_length / sizeof(multiboot_memory_map_t));

    paging_init();

    cmdline_init((void *)(VIRT_PHYS_BASE + mboot->cmdline));

    video_mode_t *vid = NULL;
	if (mboot->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO
	&& mboot->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
        vid = calloc(sizeof(video_mode_t), 1);
		vid->addr = mboot->framebuffer_addr;
		vid->pitch = mboot->framebuffer_pitch;

		vid->width = mboot->framebuffer_width;
		vid->height = mboot->framebuffer_height;
		vid->bpp = mboot->framebuffer_bpp;

		vid->red_off = mboot->framebuffer_red_field_position;
		vid->green_off = mboot->framebuffer_green_field_position;
		vid->blue_off = mboot->framebuffer_blue_field_position;

		vid->red_size = mboot->framebuffer_red_mask_size;
		vid->green_size = mboot->framebuffer_green_mask_size;
		vid->blue_size = mboot->framebuffer_blue_mask_size;
    }

    fb_init(vid);

	printf("%s %s - Kern name: %s, Build time: %s %s\n", OS_NAME, KERNEL_VERSION, KERNEL_NAME, KERNEL_DATE, KERNEL_TIME);

    while(1);
}