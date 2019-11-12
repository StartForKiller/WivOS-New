#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define VIRT_PHYS_BASE 0xFFFF800000000000UL

#define MM_FLAGS_READ			0x01
#define MM_FLAGS_WRITE			0x02
#define MM_FLAGS_EXECUTE		0x04
#define MM_FLAGS_USER			0x08
#define MM_FLAGS_NO_CACHE		0x10

typedef char ALIGN[16];

union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    ALIGN stub;
};
typedef union header header_t;

void *malloc(size_t size);
void free(void *block);
void *calloc(size_t num, size_t nsize);
void *realloc(void *block, size_t size);

#endif