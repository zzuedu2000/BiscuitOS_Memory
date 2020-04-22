#ifndef _BISCUITOS_PAGE_FLAGS_H
#define _BISCUITOS_PAGE_FLAGS_H

/*
 * Don't use the *_dontuse flags.  Use the macros.  Otherwise you'll break
 * locked- and dirty-page accounting.  The top eight bits of page->flags are
 * used for page->zone, so putting flag bits there doesn't work.
 */
#define PG_locked_bs		0	/* Page is locked. Don't touch. */
#define PG_error_bs		1
#define PG_referenced_bs	2
#define PG_uptodate_bs		3

#define PG_dirty_bs		4
#define PG_lru_bs		5
#define PG_active_bs		6
#define PG_slab_bs		7	/* slab debug (Suparna wants this) */

#define PG_highmem_bs		8
#define PG_checked_bs		9	/* kill me in 2.5.<early>. */
#define PG_arch_1_bs		10
#define PG_reserved_bs		11

#define PG_private_bs		12	/* Has something at ->private */
#define PG_writeback_bs		13	/* Page is under writeback */
#define PG_nosave_bs		14	/* Used for system suspend/resume */
#define PG_compound_bs		15	/* Part of a compound page */

#define PG_swapcache_bs		16	/* Swap page: swp_entry_t in private */
#define PG_mappedtodisk_bs	17	/* Has blocks allocated on-disk */
#define PG_reclaim_bs		18	/* To be reclaimed asap */
#define PG_nosave_free_bs	19	/* Free, should not be written */
#define PG_uncached_bs		20	/* Page has been mapped as uncached */

/*
 * Global page accounting.  One instance per CPU.  Only unsigned longs are
 * allowed.
 */
struct page_state_bs {
	unsigned long nr_dirty;         /* Dirty writeable pages */
	unsigned long nr_writeback;     /* Pages under writeback */
	unsigned long nr_unstable;      /* NFS unstable pages */
	unsigned long nr_page_table_pages;/* Pages used for pagetables */
	unsigned long nr_mapped;        /* mapped into pagetables */
	unsigned long nr_slab;          /* In slab */
#define GET_PAGE_STATE_LAST_BS	nr_slab

	/*
	 * The below are zeroed by get_page_state().  Use get_full_page_state()
	 * to add up all these.
	 */
	unsigned long pgpgin;           /* Disk reads */
	unsigned long pgpgout;          /* Disk writes */
	unsigned long pswpin;           /* swap reads */
	unsigned long pswpout;          /* swap writes */
	unsigned long pgalloc_high;     /* page allocations */
        
	unsigned long pgalloc_normal;
	unsigned long pgalloc_dma;
	unsigned long pgfree;           /* page freeings */
	unsigned long pgactivate;       /* pages moved inactive->active */
	unsigned long pgdeactivate;     /* pages moved active->inactive */

	unsigned long pgfault;          /* faults (major+minor) */
	unsigned long pgmajfault;       /* faults (major only) */
	unsigned long pgrefill_high;    /* inspected in refill_inactive_zone */
	unsigned long pgrefill_normal;
	unsigned long pgrefill_dma;

	unsigned long pgsteal_high;     /* total highmem pages reclaimed */
	unsigned long pgsteal_normal;
	unsigned long pgsteal_dma;
	unsigned long pgscan_kswapd_high;/* total highmem pages scanned */
	unsigned long pgscan_kswapd_normal;

	unsigned long pgscan_kswapd_dma;
	unsigned long pgscan_direct_high;/* total highmem pages scanned */
	unsigned long pgscan_direct_normal;
	unsigned long pgscan_direct_dma;
	unsigned long pginodesteal;     /* pages reclaimed via inode freeing */

	unsigned long slabs_scanned;    /* slab objects scanned */
	unsigned long kswapd_steal;     /* pages reclaimed by kswapd */
	unsigned long kswapd_inodesteal;/* reclaimed via kswapd inode freeing */
	unsigned long pageoutrun;       /* kswapd's calls to page reclaim */
	unsigned long allocstall;       /* direct reclaim calls */

	unsigned long pgrotated;        /* pages rotated to tail of the LRU */
	unsigned long nr_bounce;        /* pages for bounce buffers */
};

#define mod_page_state_bs(member, delta)	\
	__mod_page_state_bs(offsetof(struct page_state_bs, member), (delta))

#define inc_page_state_bs(member)	mod_page_state_bs(member, 1UL)

#define PageReserved_bs(page)		test_bit(PG_reserved_bs, &(page)->flags)
#define SetPageReserved_bs(page)	set_bit(PG_reserved_bs, &(page)->flags)
#define __ClearPageReserved_bs(page)	\
			__clear_bit(PG_reserved_bs, &(page)->flags)

#define PageDirty_bs(page)		test_bit(PG_dirty_bs, &(page)->flags)
#define ClearPageDirty_bs(page)		clear_bit(PG_dirty_bs, &(page)->flags)

#define PagePrivate_bs(page)		test_bit(PG_private_bs, \
								&(page)->flags)
#define __ClearPagePrivate_bs(page)	__clear_bit(PG_private_bs, \
								&(page)->flags)
#define __SetPagePrivate_bs(page)	__set_bit(PG_private_bs, \
								&(page)->flags)

#ifdef CONFIG_HUGETLB_PAGE_BS
#define PageCompound_bs(page)		test_bit(PG_compound_bs, &(page)->flags)
#else
#define PageCompound_bs(page)		0
#endif

#endif
