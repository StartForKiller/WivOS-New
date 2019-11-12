#include <stdint.h>
#include <kernel/vga.h>
#include <kernel/fb.h>
#include <mm/mm.h>

enum vga_colors
{
	BLACK = 0x0,
	BLUE = 0x1,
	GREEN = 0x2,
	CYAN = 0x3,
	RED = 0x4,
	MAGENTA = 0x5,
	BROWN = 0x6,
	GRAY = 0x7
};

uint8_t current_color;
unsigned int vga_cursor;
static uint8_t *vram;

void vga_init(void)
{
	int l, c;
	vram = (uint8_t *)(0xB8000 + VIRT_PHYS_BASE);

	for(l = 0; l < LINES; l++)
		for(c = 0; c < COLUMNS; c++)
			vga_put_char(' ', c, l);
}

void vga_put_char(unsigned char c, int x, int y)
{
	uint16_t attrib = (BLACK << 4) | (GRAY & 0x0F);
	vram[(y * 80 + x) * 2] = c;
	vram[(y * 80 + x) * 2 + 1] = (attrib);
}

void vga_write_char(unsigned char ch)
{
	if(ch == '\n')
		vga_newline();
	else
	{
		vga_put_char(ch, vga_cursor % COLUMNS, vga_cursor / COLUMNS);
		if(vga_cursor >= 80 + 25 * 80)
			vga_cursor = 0;
		else
			vga_cursor++;
	}
}

void vga_write_string(char* str)
{
	while(*str != '\0')
	{
		fb_putch(str[0]);
		vga_write_char(str[0]);
		str++;
	}
}

void vga_newline()
{
	vga_cursor += COLUMNS - (vga_cursor % COLUMNS);
	if(vga_cursor >= 80 + 25 * 80)
			vga_cursor = 0;
}