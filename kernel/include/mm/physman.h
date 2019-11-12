#ifndef PHYSMAN_H
#define PHYSMAN_H

#include <stddef.h>
#include <stdint.h>
#include <multiboot.h>

void physman_init(multiboot_memory_map_t *mmap, size_t mmap_length);
void *physman_alloc_ex(size_t count, size_t alignment, uintptr_t upper);
void *physman_alloc(size_t count);
void physman_free(void *mem, size_t count);

#endif