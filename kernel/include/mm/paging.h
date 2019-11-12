#ifndef _PAGING_H_
#define _PAGING_H_

#include <stdint.h>
#include <stddef.h>

#define PAGING_FLAG_MASK 0xFFF
#define PAGING_ADDR_MASK ~(PAGING_FLAG_MASK)

#define PAGING_FLAG_PRESENT	    (1<<0)
#define PAGING_FLAG_WRITE		(1<<1)
#define PAGING_FLAG_USER		(1<<2)
#define PAGING_FLAG_WT			(1<<3)
#define PAGING_FLAG_NO_CACHE	(1<<4)
#define PAGING_FLAG_DIRTY		(1<<5)
#define PAGING_FLAG_LARGE		(1<<7)

typedef struct {
	uint64_t ents[512];
} pt_t;

typedef struct {
	size_t pml4_off;
	size_t pdp_off;
	size_t pd_off;
	size_t pt_off;
} pt_off_t;

pt_off_t paging_virt_to_offs(void *);
void *paging_offs_to_virt(pt_off_t);

void paging_init(void);

int paging_map_pages(pt_t *, void *, void *, size_t, int);
int paging_unmap_pages(pt_t *, void *, size_t);
int paging_update_perms(pt_t *, void *, size_t, int);

pt_t *paging_new_address_space(void);

void paging_save_pml4(void);
pt_t *paging_get_saved_pml4(void);
void paging_restore_pml4(void);
void paging_set_pml4(pt_t *);
pt_t *paging_get_current_pml4(void);

void paging_update_mapping(void *ptr);

int paging_mm_map_kernel(void *dst, void *src, size_t size, int flags);

#endif