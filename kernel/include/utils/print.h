#ifndef _PRINT_H_
#define _PRINT_H_

void print(char* string);
void print_hex64(uint64_t num);
void kprint(const char *src, const char *fmt, ...);
void printf(const char *fmt, ...);
void panic(const char *message, ...);

#endif