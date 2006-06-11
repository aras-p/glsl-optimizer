#ifndef __RADEON_MM_H__
#define __RADEON_MM_H__

//#define RADEON_MM_PDL 0
#define RADEON_MM_UL 1

#define RADEON_MM_R 1
#define RADEON_MM_W 2
#define RADEON_MM_RW (RADEON_MM_R | RADEON_MM_W)

#define RADEON_MM_SCRATCH 2

struct radeon_memory_manager {
	struct {
		void *ptr;
		uint32_t size;
		uint32_t age;
		uint32_t h_pending;
		int pending;
		int mapped;
		int ref_count;
		struct mem_block *fb;
	} *u_list;
	int u_head, u_tail, u_size, u_last;
	
};

extern void radeon_mm_init(r300ContextPtr rmesa);
extern void radeon_mm_destroy(r300ContextPtr rmesa);
extern void *radeon_mm_ptr(r300ContextPtr rmesa, int id);
extern int radeon_mm_find(r300ContextPtr rmesa, void *ptr);
extern int radeon_mm_alloc(r300ContextPtr rmesa, int alignment, int size);
extern void radeon_mm_use(r300ContextPtr rmesa, int id);
extern unsigned long radeon_mm_offset(r300ContextPtr rmesa, int id);
extern int radeon_mm_on_card(r300ContextPtr rmesa, int id);
extern void *radeon_mm_map(r300ContextPtr rmesa, int id, int access);
extern void radeon_mm_unmap(r300ContextPtr rmesa, int id);
extern void radeon_mm_free(r300ContextPtr rmesa, int id);

#endif
