#ifndef _BISCUITOS_MM_H
#define _BISCUITOS_MM_H

#include <linux/mm_types.h>
#include "biscuitos/mmzone.h"
#include "biscuitos/gfp.h"
#include "asm-generated/memory.h"
#include "asm-generated/pgtable.h"
#include "asm-generated/page.h"

#define VM_NORESERVE_BS		0x00200000     /* should the VM suppress accounting */
#define VM_RESERVED_BS		0x00080000    /* Don't unmap it from swap_out */
#define VM_ACCOUNT_BS		0x00100000    /* Is a VM accounted object */

#ifdef ARCH_HAS_ATOMIC_UNSIGNED
typedef unsigned page_flags_t_bs;
#else
typedef unsigned long page_flags_t_bs;
#endif

/*
 * The zone field is never updated after free_area_init_core()
 * sets it, so none of the operations on it need to be atomic.
 * We'll have up to (MAX_NUMNODES * MAX_NR_ZONES) zones total,
 * so we use (MAX_NODES_SHIFT + MAX_ZONES_SHIFT) here to get enough bits.
 */
#define NODEZONE_SHIFT_BS	(sizeof(page_flags_t_bs)*8 - \
				 MAX_NODES_SHIFT_BS - MAX_ZONES_SHIFT_BS)
#define NODEZONE_BS(node, zone)	((node << ZONES_SHIFT_BS) | zone)

struct address_space;

/*
 * Each physical page in the system has a struct page associated with
 * it to keep track of whatever it is we are using the page for at the
 * moment. Note that we have no way to track which tasks are using
 * a page.
 */
struct page_bs {
	page_flags_t_bs flags;          /* Atomic flags, some possibly
					 * updated asynchronously */
	atomic_t _count;                /* Usage count, see below. */
	atomic_t _mapcount;             /* Count of ptes mapped in mms,
					 * to show when page is mapped
					 * & limit reverse map searches.
					 */
	unsigned long private;          /* Mapping-private opaque data:
					 * usually used for buffer_heads
					 * if PagePrivate set; used for
					 * swp_entry_t if PageSwapCache
					 * When page is free, this indicates
					 * order in the buddy system.
					 */
	struct address_space *mapping; /* If low bit clear, points to
					 * inode address_space, or NULL.
					 * If page mapped as anonymous
					 * memory, low bit is set, and
					 * it points to anon_vma object:
					 * see PAGE_MAPPING_ANON below.
					 */
	pgoff_t index;                  /* Our offset within mapping. */
	struct list_head lru;           /* Pageout list, eg. active_list
					 * protected by zone->lru_lock !
					 */
	/*
	 * On machines where all RAM is mapped into kernel address space,
	 * we can simply calculate the virtual address. On machines with
	 * highmem some memory is mapped into kernel virtual memory
	 * dynamically, so we need a place to store that address.
	 * Note that this field could be 16 bits on x86 ... ;)
	 *
	 * Architectures with slow multiplication can define
	 * WANT_PAGE_VIRTUAL in asm/page.h
	 */
#if defined(WANT_PAGE_VIRTUAL)
	void *virtual;                  /* Kernel virtual address (NULL if
					   not kmapped, ie. highmem) */
#endif /* WANT_PAGE_VIRTUAL */
};

/* FIXME: */
struct shmem_page {
	struct page page;
	struct page_bs page_bs;
};

/*
 * FIXME: take this include out, include page-flags.h in
 * files which need it (119 of them)
 */
#include "biscuitos/page-flags.h"

/*
 * Drop a ref, return true if the logical refcount fell to zero (the page has
 * no users)
 */
#define put_page_testzero_bs(p)						\
({									\
	BUG_ON_BS(page_count_bs(p) == 0);				\
	atomic_add_negative(-1, &(p)->_count);				\
})

/*
 * Grab a ref, return true if the page previously had a logical refcount of
 * zero.  ie: returns true if we just grabbed an already-deemed-to-be-free page
 */
#define get_page_testone_bs(p)	atomic_inc_and_test(&(p)->_count)

#define set_page_count_bs(p,v)	atomic_set(&(p)->_count, v - 1)
#define __put_page_bs(p)	atomic_dec(&(p)->_count)

static inline int page_count_bs(struct page_bs *p)
{
	if (PageCompound_bs(p))
		p = (struct page_bs *)p->private;
	return atomic_read(&(p)->_count) + 1;
}

#ifndef CONFIG_DISCONTIGMEM
/* The array of struct pages - for discontigmen use pgdat->lmem_map */
extern struct page_bs *mem_map_bs;
#endif

static inline void set_page_zone_bs(struct page_bs *page, 
					unsigned long nodezone_num)
{
	page->flags &= ~(~0UL << NODEZONE_SHIFT_BS);
	page->flags |= nodezone_num << NODEZONE_SHIFT_BS;
}

#define set_page_count_bs(p, v)		atomic_set(&(p)->_count, v - 1)

/*      
 * The atomic page->_mapcount, like _count, starts from -1:
 * so that transitions both from it and to it can be tracked,
 * using atomic_inc_and_test and atomic_add_negative(-1). 
 */
static inline void reset_page_mapcount_bs(struct page_bs *page)
{
	atomic_set(&(page)->_mapcount, -1);
}

static inline void page_alloc_init_bs(void)
{
}

/*      
 * On an anonymous page mapped into a user virtual memory area,
 * page->mapping points to its anon_vma, not to a struct address_space;
 * with the PAGE_MAPPING_ANON bit set to distinguish it.
 *              
 * Please note that, confusingly, "page_mapping" refers to the inode
 * address_space which maps the page from disk; whereas "page_mapped"
 * refers to user virtual address space into which the page is mapped.
 */
#define PAGE_MAPPING_ANON_BS	1

static inline int PageAnon_bs(struct page_bs *page)
{
	return ((unsigned long)page->mapping & PAGE_MAPPING_ANON_BS) != 0;
}

/*
 * Return true if this page is mapped into pagetables.
 */
static inline int page_mapped_bs(struct page_bs *page)
{
	return atomic_read(&(page)->_mapcount) >= 0;
}

struct pglist_data_bs;
extern void *high_memory_bs;
extern unsigned long num_physpages_bs;
extern pgprot_t_bs protection_map_bs[16];
extern void __init free_area_init_node_bs(int nid, struct pglist_data_bs *pgdat,
		unsigned long *zones_size, unsigned long node_start_pfn,
		unsigned long *zhole_size);
extern void __init page_address_init_bs(void);
extern void __init mem_init_bs(void);

struct zone_bs;
extern struct zone_bs *zone_table_bs[];

static inline struct zone_bs *page_zone_bs(struct page_bs *page)
{
	return zone_table_bs[page->flags >> NODEZONE_SHIFT_BS];
}

#ifndef CONFIG_DEBUG_PAGEALLOC_BS
static inline void
kernel_map_pages_bs(struct page_bs *page, int numpages, int enable)
{
}
#endif

static inline int page_mapcount_bs(struct page_bs *page)
{
	return atomic_read(&(page)->_mapcount) + 1;
}

void *page_address_bs(struct page_bs *page);
void set_page_address_bs(struct page_bs *page, void *virtual);

static inline void *lowmem_page_address_bs(struct page_bs *page)
{
	return __va_bs(page_to_pfn_bs(page) << PAGE_SHIFT_BS);
}

extern unsigned long num_physpages_bs;
extern pte_t_bs *FASTCALL_BS(pte_alloc_kernel_bs(struct mm_struct_bs *mm,
				pmd_t_bs *pmd, unsigned long address));
extern void show_free_areas_bs(void);
struct sysinfo_bs;
extern void si_meminfo_bs(struct sysinfo_bs *);
extern int __set_page_dirty_nobuffers_bs(struct page *page);
extern void PAGE_TO_PAGE_BS(struct page *page, struct page_bs *page_bs);
extern void PAGE_BS_TO_PAGE(struct page *page, struct page_bs *page_bs);

#endif
