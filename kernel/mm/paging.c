#include <stdint.h>
#include <mm/paging.h>
#include <mm/physman.h>
#include <mm/mm.h>
#include <utils/print.h>
#include <utils/string.h>

pt_off_t paging_virt_to_offs(void *virtual) {
	uintptr_t address = (uintptr_t)virtual;

	pt_off_t offset = {
		.pml4_off = (address & ((size_t)0x1ff << 39)) >> 39,
		.pdp_off = (address & ((size_t)0x1ff << 30)) >> 30,
		.pd_off = (address & ((size_t)0x1ff << 21)) >> 21,
		.pt_off = (address & ((size_t)0x1ff << 12)) >> 12,
	};

	return offset;
}

void *paging_offs_to_virt(pt_off_t offset) {
	uintptr_t address = 0;

	address |= offset.pml4_off << 39;
	address |= offset.pdp_off << 30;
	address |= offset.pd_off << 21;
	address |= offset.pt_off << 12;

	return (void *)address;
}

static pt_t *kernel_pml4;
static pt_t *paging_saved_plm4 = NULL;

void paging_init(void)
{
	kernel_pml4 = paging_new_address_space();

	paging_set_pml4(kernel_pml4);

	paging_saved_plm4 = NULL;
}

static inline pt_t *paging_get_or_alloc_ent(pt_t *tab, size_t off, int flags) {
	uint64_t ent_addr = tab->ents[off] & PAGING_ADDR_MASK;
	if(!ent_addr) {
		ent_addr = tab->ents[off] = (uint64_t)physman_alloc(1);
		if(!ent_addr) {
			return NULL;
		}
		tab->ents[off] |= flags | PAGING_FLAG_PRESENT;
		memset((void *)(ent_addr + VIRT_PHYS_BASE), 0, 4096);
	}

	return (pt_t *)(ent_addr + VIRT_PHYS_BASE);
}

static inline pt_t *paging_get_or_null_ent(pt_t *tab, size_t off) {
	uint64_t ent_addr = tab->ents[off] & PAGING_ADDR_MASK;
	if (!ent_addr) {
		return NULL;
	}

	return (pt_t *)(ent_addr + VIRT_PHYS_BASE);
}

int paging_map_pages(pt_t *pml4, void *virt, void *phys, size_t count, int perms) {
	while(count--) {
		pt_off_t offset = paging_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = paging_get_or_alloc_ent(pml4_virt, offset.pml4_off, perms);
		pt_t *pd_virt = paging_get_or_alloc_ent(pdp_virt, offset.pdp_off, perms);
		pt_t *pt_virt = paging_get_or_alloc_ent(pd_virt, offset.pd_off, perms);
		pt_virt->ents[offset.pt_off] = (uint64_t)phys | perms | PAGING_FLAG_PRESENT;

		virt = (void*)((uintptr_t)virt + PAGE_SIZE);
		phys = (void*)((uintptr_t)phys + PAGE_SIZE);
	}

	return 1;
}

int paging_unmap_pages(pt_t *pml4, void *virt, size_t count) {
	while(count--) {
		pt_off_t offset = paging_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = paging_get_or_null_ent(pml4_virt, offset.pml4_off);
		if(!pdp_virt) return 0;
		pt_t *pd_virt = paging_get_or_null_ent(pdp_virt, offset.pdp_off);
		if(!pd_virt) return 0;
		pt_t *pt_virt = paging_get_or_null_ent(pd_virt, offset.pd_off);
		if(!pt_virt) return 0;
		pt_virt->ents[offset.pt_off] = 0;

		virt = (void*)((uintptr_t)virt + PAGE_SIZE);
	}

	return 1;
}

int paging_update_perms(pt_t *pml4, void *virt, size_t count, int perms) {
	while(count--) {
		pt_off_t offset = paging_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = paging_get_or_null_ent(pml4_virt, offset.pml4_off);
		if(!pdp_virt) return 0;
		pt_t *pd_virt = paging_get_or_null_ent(pdp_virt, offset.pdp_off);
		if(!pd_virt) return 0;
		pt_t *pt_virt = paging_get_or_null_ent(pd_virt, offset.pd_off);
		if(!pt_virt) return 0;
		pt_virt->ents[offset.pt_off] = (pt_virt->ents[offset.pt_off] & PAGING_ADDR_MASK) | perms | PAGING_FLAG_PRESENT;

		virt = (void*)((uintptr_t)virt + PAGE_SIZE);
	}

	return 1;
}

int paging_map_huge_pages(pt_t *pml4, void *virt, void *phys, size_t count, int perms) {
	while(count--) {
		pt_off_t offset = paging_virt_to_offs(virt);

		pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
		pt_t *pdp_virt = paging_get_or_alloc_ent(pml4_virt, offset.pml4_off, perms);
		pt_t *pd_virt = paging_get_or_alloc_ent(pdp_virt, offset.pdp_off, perms);
		pd_virt->ents[offset.pd_off] = (uint64_t)phys | perms | PAGING_FLAG_PRESENT | PAGING_FLAG_LARGE;

		virt = (void*)((uintptr_t)virt + 0x200000);
		phys = (void*)((uintptr_t)phys + 0x200000);
	}

	return 1;
}

uintptr_t paging_get_entry(pt_t *pml4, void *virt) {
	if ((uintptr_t)virt >= 0xFFFFFFFF80000000)
		return (uintptr_t)virt - 0xFFFFFFFF80000000; 

	if ((uintptr_t)virt >= 0xFFFF800000000000)
		return (uintptr_t)virt - 0xFFFF800000000000;

	pt_off_t offs = paging_virt_to_offs(virt);

	pt_t *pml4_virt = (pt_t *)((uint64_t)pml4 + VIRT_PHYS_BASE);
	pt_t *pdp_virt = paging_get_or_null_ent(pml4_virt, offs.pml4_off);
	if (!pdp_virt) return 0;
	pt_t *pd_virt = paging_get_or_null_ent(pdp_virt, offs.pdp_off);
	if (!pd_virt) return 0;
	pt_t *pt_virt = paging_get_or_null_ent(pd_virt, offs.pd_off);
	if (!pt_virt) return 0;
	return pt_virt->ents[offs.pt_off];
}

pt_t *paging_new_address_space() {
	pt_t *new_pml4 = physman_alloc(1);
	memset((void *)((uintptr_t)new_pml4 + VIRT_PHYS_BASE), 0, 4096);

	paging_map_huge_pages(new_pml4, (void *)0xFFFFFFFF80000000, NULL, 64, 3);
	paging_map_huge_pages(new_pml4, (void *)0xFFFF800000000000, NULL, 512 * 5, 3);

	return new_pml4;
}

void paging_ctx_memcpy(pt_t *dst_ctx, void *dst_addr, pt_t *src_ctx, void *src_addr, size_t size) {
	uintptr_t src_virt = 0x780000000000;
	uintptr_t dst_virt = 0x700000000000;
	uintptr_t dst = (uintptr_t)dst_addr & (~0xFFF);
	uintptr_t src = (uintptr_t)src_addr & (~0xFFF);
	for (size_t i = 0; i < size;
		i += 0x1000, src_virt += 0x1000, src += 0x1000,
			dst_virt += 0x1000, dst += 0x1000) {
		uintptr_t dst_phys = paging_get_entry(dst_ctx, (void *)dst)
				& PAGING_ADDR_MASK;
		uintptr_t src_phys = paging_get_entry(src_ctx, (void *)src)
				& PAGING_ADDR_MASK;

		paging_map_pages(kernel_pml4, (void *)dst_virt, (void *)dst_phys,
					1, PAGING_FLAG_WRITE);
		paging_map_pages(kernel_pml4, (void *)src_virt, (void *)src_phys,
					1, PAGING_FLAG_WRITE);
		paging_update_mapping((void *)dst_virt);
		paging_update_mapping((void *)src_virt);
	}

	memcpy((void *)(0x700000000000 + ((uintptr_t)dst_addr & 0xFFF)),
		(void *)(0x780000000000 + ((uintptr_t)src_addr & 0xFFF)), size);

	src_virt = 0x780000000000;
	dst_virt = 0x700000000000;
	for (size_t i = 0; i < size;
		i += 0x1000, src_virt += 0x1000, dst_virt += 0x1000) {
		paging_unmap_pages(kernel_pml4, (void *)dst_virt, 1);
		paging_unmap_pages(kernel_pml4, (void *)src_virt, 1);
		paging_update_mapping((void *)dst_virt);
		paging_update_mapping((void *)src_virt);
	}
}

void paging_ctx_memcpy2(pt_t *dst_ctx, void *dst_addr, pt_t *src_ctx, void *src_addr, size_t size) {
	uintptr_t src_virt = 0x780000000000;
	uintptr_t dst_virt = 0x700000000000;
	uintptr_t dst = (uintptr_t)dst_addr & (~0xFFF);
	uintptr_t src = (uintptr_t)src_addr & (~0xFFF);
	for (size_t i = 0; i < size;
		i += 0x1000, src_virt += 0x1000, src += 0x1000,
			dst_virt += 0x1000, dst += 0x1000) {
		uintptr_t dst_phys = paging_get_entry(dst_ctx, (void *)dst)
				& PAGING_ADDR_MASK;
		uintptr_t src_phys = paging_get_entry(src_ctx, (void *)src)
				& PAGING_ADDR_MASK;

		paging_map_pages(src_ctx, (void *)dst_virt, (void *)dst_phys,
					1, PAGING_FLAG_WRITE);
		paging_map_pages(src_ctx, (void *)src_virt, (void *)src_phys,
					1, PAGING_FLAG_WRITE);
		paging_update_mapping((void *)dst_virt);
		paging_update_mapping((void *)src_virt);
	}

	memcpy((void *)(0x700000000000 + ((uintptr_t)dst_addr & 0xFFF)),
		(void *)(0x780000000000 + ((uintptr_t)src_addr & 0xFFF)), size);

	src_virt = 0x780000000000;
	dst_virt = 0x700000000000;
	for (size_t i = 0; i < size;
		i += 0x1000, src_virt += 0x1000, dst_virt += 0x1000) {
		paging_unmap_pages(src_ctx, (void *)dst_virt, 1);
		paging_unmap_pages(src_ctx, (void *)src_virt, 1);
		paging_update_mapping((void *)dst_virt);
		paging_update_mapping((void *)src_virt);
	}
}

pt_t *fork_addr_space(pt_t *old_pt) {
	pt_t *new_pt = paging_new_address_space();

	struct {
        uint8_t data[PAGE_SIZE];
    } *pool;

    size_t pool_size = 4096;
    pool = physman_alloc(pool_size);
    size_t pool_ptr = 0;

	pt_t *pml4_virt = (pt_t *)((uint64_t)old_pt + VIRT_PHYS_BASE);
	for(size_t i = 0; i < 512/2; i++) {
		if(!(pml4_virt->ents[i] & 1)) continue;
		pt_t *pdp_virt = paging_get_or_null_ent(pml4_virt, i);
		if (!pdp_virt) continue;
		for(size_t j = 0; j < 512; j++) {
			if(!(pdp_virt->ents[j] & 1)) continue;
			pt_t *pd_virt = paging_get_or_null_ent(pdp_virt, j);
			if (!pd_virt) continue;
			for(size_t k = 0; k < 512; k++) {
				if(!(pd_virt->ents[k] & 1)) continue;
				pt_t *pt_virt = paging_get_or_null_ent(pd_virt, k);
				if (!pt_virt) continue;
				for(size_t l = 0; l < 512; l++) {
					if(!(pt_virt->ents[l] & 1)) continue;
					if(pool_ptr == pool_size) {
						pool = physman_alloc(pool_size);
						pool_ptr = 0;
						//printf("Uh! %lx", pool_ptr);
					}
					size_t new_page = (size_t)&pool[pool_ptr++];
					memcpy((char *)(new_page + VIRT_PHYS_BASE),
						(char *)((pt_virt->ents[l] & 0xfffffffffffff000) + VIRT_PHYS_BASE), PAGE_SIZE);
					size_t virt_addr = i << 39 | j << 30 | k << 21 | l << 12;
					paging_map_pages(new_pt, (void *)virt_addr, (void *)new_page, 1, pt_virt->ents[l] & 0xfff);
				}
			}
		}
	}
	
	physman_free(pool + pool_ptr, pool_size - pool_ptr);

	pt_t *pml4_virt2 = (pt_t *)((uint64_t)new_pt + VIRT_PHYS_BASE);
	for(size_t i = 512 / 2; i < 512; i++) {
		pml4_virt2->ents[i] = pml4_virt->ents[i];
	}

	return new_pt;
}

void paging_save_pml4(void) {
	paging_saved_plm4 = paging_get_current_pml4();
}

pt_t *paging_get_saved_pml4(void) {
	return paging_saved_plm4;
}

void paging_restore_pml4(void) {
	paging_set_pml4(paging_saved_plm4);
	paging_saved_plm4 = NULL;
}

void paging_set_pml4(pt_t *ctx) {
	asm volatile ("mov %%rax, %%cr3" : : "a"(ctx) : "memory");
}

pt_t *paging_get_current_pml4(void) {
	uintptr_t ctx = 0;
	asm volatile ("mov %%cr3, %%rax" : "=a"(ctx) : : "memory");
	return (pt_t *)ctx;
}

void paging_update_mapping(void *ptr) {
	asm volatile ("invlpg (%0)" : : "r"(ptr) : "memory");
}

int paging_mm_to_paging_flags(int flags) {
	return ((flags & MM_FLAGS_WRITE) ? PAGING_FLAG_WRITE : 0)
		| ((flags & MM_FLAGS_USER) ? PAGING_FLAG_USER : 0)
		| ((flags & MM_FLAGS_NO_CACHE) ?
			(PAGING_FLAG_NO_CACHE | PAGING_FLAG_WT) : 0);
}

int paging_mm_map_kernel(void *dst, void *src, size_t size, int flags) {
	return paging_map_pages(kernel_pml4, dst, src, size, paging_mm_to_paging_flags(flags));
}

void *mm_get_ctx_kernel(int cpu) {
	(void)cpu;
	return kernel_pml4;
}