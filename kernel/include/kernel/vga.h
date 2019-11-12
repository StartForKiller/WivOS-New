#ifndef _VGA_H_
#define _VGA_H_

#include <stdint.h>
#include <mm/mm.h>

#define VGA_MEM_START 0xB8000 + VIRT_PHYS_BASE
#define LINES 25
#define COLUMNS 80

void vga_init(void);
void vga_put_char(unsigned char c, int x, int y);
void vga_write_char(unsigned char ch);
void vga_write_string(char* str);
void vga_newline();

#endif