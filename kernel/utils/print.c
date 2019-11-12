#include <stdint.h>
#include <kernel/vga.h>
#include <kernel/fb.h>
#include <utils/string.h>
#include <utils/vsnprintf.h>
#include <utils/spinlock.h>
#include <utils/port.h>

static spinlock_t plock;

void print(char* string)
{
	vga_write_string(string);
}

int
serial_transmit_empty() {
	return inb(0x3F8 + 5) & 0x20;
}

void
serial_send(char out) {
	while (serial_transmit_empty() == 0);
	outb(0x3F8, out);
}

void print_char(char c)
{
	fb_putch(c);
	vga_write_char(c);
	serial_send(c);
}

void print_hex64(uint64_t num)
{
	char str[20];

	itoa(num, str, 16);
	vga_write_string(str);
}

void kprint(const char *src, const char *fmt, ...) {
	spinlock_lock(&plock);

	char fmt_buf[1024 + 16];
	va_list va;
	va_start(va, fmt);
	vsnprintf(fmt_buf, 1024, fmt, va);
	va_end(va);

	char out_buf[1162 + 16];
	snprintf(out_buf, 1162, "[%s] %s\n", src, fmt_buf);

	char *out = out_buf;
	while(*out)
		print_char(*out++);

	spinlock_release(&plock);
}

void printf(const char *fmt, ...) {
	spinlock_lock(&plock);

	char fmt_buf[1024 + 16];
	va_list va;
	va_start(va, fmt);
	vsnprintf(fmt_buf, 1024, fmt, va);
	va_end(va);

	char *out = fmt_buf;
	while(*out)
		print_char(*out++);

	spinlock_release(&plock);
}


void panic(const char *message, ...) {
	char buf[128 + 16];
	va_list va;
	memset(buf, 0, 128 + 16);

	va_start(va, message);
	vsnprintf(buf, 128, message, va);
	va_end(va);

	kprint("KERNEL", "Kernel panic!");
	kprint("KERNEL", "Message: '%s'", buf);

	asm volatile ("cli\n"
				  "1:\n"
					"hlt\n"
					"jmp 1b" : : : "memory");
}
