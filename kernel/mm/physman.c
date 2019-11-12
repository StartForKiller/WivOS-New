#include <mm/physman.h>
#include <mm/mm.h>
#include <utils/print.h>
#include <utils/string.h>
#include <stddef.h>
#include <stdint.h>
#include <multiboot.h>

#define PHYSMAN_MEMORY_BASE 0x2000000

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N,S) ((N / S) * S)
#define OVERLAPS(a, as, b, bs) ((a) >= (b) && (a + as) <= (b + bs))

uint64_t *physman_bitmap = NULL;
size_t physman_bitmap_length = 64;

static size_t physman_free_pages = 0;
static size_t physman_total_pages = 0;

static int bitmap_read_bit(int pageNumber)
{
    size_t offset = pageNumber / 64;
    size_t mask = (1 << (pageNumber % 64));

    return (physman_bitmap[offset] & mask) == mask;
}

static void bitmap_write_bit(int pageNumber, int bit, size_t count)
{
    for(; count; count--, pageNumber++) {
        size_t offset = pageNumber / 64;
        size_t mask = (1 << (pageNumber % 64));

        if(bit)
            physman_bitmap[offset] |= mask;
        else
            physman_bitmap[offset] &= ~mask;
    }
}

static int find_free_page(size_t idx, size_t count)
{
    for(; count; count--, idx++) {
        if(bitmap_read_bit(idx))
            return 0;
    }
    return 1;
}

static uintptr_t physman_find_memory_top(multiboot_memory_map_t *mmap, size_t mmap_length)
{
    uintptr_t mem_top = 0;
    for(size_t i = 0; i < mmap_length; i++) {
        if(mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if(mmap[i].addr + mmap[i].len > mem_top)
                mem_top = mmap[i].addr + mmap[i].len;
        }
    }

    return mem_top;
}

void physman_init(multiboot_memory_map_t *mmap, size_t mmap_length)
{
    uintptr_t mem_top = physman_find_memory_top(mmap, mmap_length);
    uint32_t mem_pages = (mem_top + PAGE_SIZE - 1) / PAGE_SIZE;

    physman_bitmap = (uint64_t *)(PHYSMAN_MEMORY_BASE + VIRT_PHYS_BASE);
    physman_bitmap_length = mem_pages;

    size_t bitmap_phys = (size_t)physman_bitmap - VIRT_PHYS_BASE;

    memset(physman_bitmap, 0xFF, physman_bitmap_length / 8);

    for (size_t i = 0; i < mmap_length; i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            uintptr_t start = ROUND_UP(mmap[i].addr, PAGE_SIZE);
			size_t len = ROUND_DOWN(mmap[i].len, PAGE_SIZE);
			size_t count = len / PAGE_SIZE;

            if (!len) continue;
			if (start + len < PHYSMAN_MEMORY_BASE) continue;

            if (start < PHYSMAN_MEMORY_BASE) {
				len -= PHYSMAN_MEMORY_BASE - start;
				start = PHYSMAN_MEMORY_BASE;
				count = len / PAGE_SIZE;
			}

            if (OVERLAPS(bitmap_phys, physman_bitmap_length / 8, start, len)) {
                if (start < bitmap_phys)
					physman_free((void *)start, (start - bitmap_phys) / PAGE_SIZE);

				start = bitmap_phys + physman_bitmap_length / 8;
				len -= physman_bitmap_length / 8;
				count = len / PAGE_SIZE;
            }

            physman_free((void *)start, count);
        }
    }

    physman_total_pages = physman_free_pages;

    bitmap_write_bit(bitmap_phys / PAGE_SIZE, 1, (physman_bitmap_length / 8 + PAGE_SIZE - 1) / PAGE_SIZE);

    physman_alloc((physman_bitmap_length / 8 + PAGE_SIZE - 1) / PAGE_SIZE);
    printf("Total pages free: %lu\n", physman_free_pages);
}

void *physman_alloc_ex(size_t count, size_t alignment, uintptr_t upper) {
    size_t idx = PHYSMAN_MEMORY_BASE / PAGE_SIZE, max_idx = 0;

    if(!upper)
        max_idx = physman_bitmap_length;
    else
        max_idx = physman_bitmap_length < (upper / PAGE_SIZE) ? physman_bitmap_length : (upper / PAGE_SIZE);
    
    while(idx < max_idx) {
        if(!find_free_page(idx, count)) {
            idx += alignment;
            continue;
        }
        bitmap_write_bit(idx, 1, count);
        if (physman_total_pages)
            physman_free_pages -= count;

        uint64_t *pages = (uint64_t *)((idx * PAGE_SIZE) + VIRT_PHYS_BASE);
        for (size_t i = 0; i < (count * PAGE_SIZE) / sizeof(uint64_t); i++)
            pages[i] = 0;
        return (void *)(idx * PAGE_SIZE);
    }

    return NULL;
}

void *physman_alloc(size_t count) {
    return physman_alloc_ex(count, 1, 0);
}

void physman_free(void *mem, size_t count) {
    size_t idx = (size_t)mem / PAGE_SIZE;
    bitmap_write_bit(idx, 0, count);
    physman_free_pages += count;
}