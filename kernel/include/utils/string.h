#ifndef _STRING_H_
#define _STRING_H_

#include <stddef.h>

char* itoa(int num, char* str, int base);
void reverse(char str[], int length);

size_t strlen(const char *);
int strcmp(const char *str1, const char *str2);
char *strcpy(char *, const char *);

char *strchr(char *, int);
char * strdup(const char * s);

void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);

void *memchr(void *, int, size_t);

__attribute__((weak))
void mem_fast_memcpy(void *dst, void *src, size_t count);
__attribute__((weak))
void mem_fast_memset(void *dst, int val, size_t count);

#endif