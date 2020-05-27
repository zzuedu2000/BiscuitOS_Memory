#ifndef _BISCUITOS_GFP_H
#define _BISCUITOS_GFP_H

#include "biscuitos/mmzone.h"
#include "biscuitos/linkage.h"
#include "biscuitos/types.h"
#include "biscuitos/kernel.h"

/*
 * GFP bitmasks..
 */
/* Zone modifiers in GFP_ZONEMASK (see linux/mmzone.h - low three bits) */
#define __GFP_DMA_BS		((__force gfp_t_bs)0x01u)
#define __GFP_HIGHMEM_BS	((__force gfp_t_bs)0x02u)
#define __GFP_DMA32_BS		((__force gfp_t_bs)0x04u) /* Has own ZONE_DMA32 */

/*
 * Action modifiers - doesn't change the zoning
 *
 * __GFP_REPEAT: Try hard to allocate the memory, but the allocation attempt
 * _might_ fail.  This depends upon the particular VM implementation.
 *
 * __GFP_NOFAIL: The VM implementation _must_ retry infinitely: the caller
 * cannot handle allocation failures.
 *
 * __GFP_NORETRY: The VM implementation must not retry indefinitely.
 */
#define __GFP_WAIT_BS		((__force gfp_t_bs)0x10u)	/* Can wait and reschedule? */
#define __GFP_HIGH_BS		((__force gfp_t_bs)0x20u)	/* Should access emergency pools? */
#define __GFP_IO_BS		((__force gfp_t_bs)0x40u)	/* Can start physical IO? */
#define __GFP_FS_BS		((__force gfp_t_bs)0x80u)	/* Can call down to low-level FS? */
#define __GFP_COLD_BS		((__force gfp_t_bs)0x100u)	/* Cache-cold page required */
#define __GFP_NOWARN_BS		((__force gfp_t_bs)0x200u)	/* Suppress page allocation failure warning */
#define __GFP_REPEAT_BS		((__force gfp_t_bs)0x400u)	/* Retry the allocation.  Might fail */
#define __GFP_NOFAIL_BS		((__force gfp_t_bs)0x800u)	/* Retry for ever.  Cannot fail */
#define __GFP_NORETRY_BS	((__force gfp_t_bs)0x1000u)/* Do not retry.  Might fail */
#define __GFP_NO_GROW_BS	((__force gfp_t_bs)0x2000u)/* Slab internal usage */
#define __GFP_COMP_BS		((__force gfp_t_bs)0x4000u)/* Add compound page metadata */
#define __GFP_ZERO_BS		((__force gfp_t_bs)0x8000u)/* Return zeroed page on success */
#define __GFP_NOMEMALLOC_BS	((__force gfp_t_bs)0x10000u) /* Don't use emergency reserves */
#define __GFP_HARDWALL_BS	((__force gfp_t_bs)0x20000u) /* Enforce hardwall cpuset memory allocs */


#define __GFP_BITS_SHIFT_BS	20     /* Room for 20 __GFP_FOO bits */
#define __GFP_BITS_MASK_BS	 ((__force gfp_t_bs)((1 << \
						__GFP_BITS_SHIFT_BS) - 1))

/* if you forget to add the bitmask here kernel will crash, period */
#define GFP_LEVEL_MASK_BS	(__GFP_WAIT_BS | __GFP_HIGH_BS | \
				 __GFP_IO_BS | __GFP_FS_BS | \
				 __GFP_FS_BS | __GFP_COLD_BS | \
				 __GFP_NOWARN_BS | __GFP_REPEAT_BS | \
				 __GFP_NOFAIL_BS | __GFP_NORETRY_BS | \
				 __GFP_NO_GROW_BS | __GFP_COMP_BS | \
				 __GFP_NOMEMALLOC_BS | __GFP_HARDWALL_BS)

/* GFP_ATOMIC means both !wait (__GFP_WAIT not set) and use emergency pool */
#define GFP_ATOMIC_BS		(__GFP_HIGH_BS)
#define GFP_NOIO_BS		(__GFP_WAIT_BS)
#define GFP_NOFS_BS		(__GFP_WAIT_BS | __GFP_IO_BS)
#define GFP_KERNEL_BS		(__GFP_WAIT_BS | __GFP_IO_BS | __GFP_FS_BS)
#define GFP_USER_BS		(__GFP_WAIT_BS | __GFP_IO_BS | __GFP_FS_BS | \
						__GFP_HARDWALL_BS)
#define GFP_HIGHUSER_BS		(__GFP_WAIT_BS | __GFP_IO_BS | __GFP_FS_BS | \
					__GFP_HIGHMEM_BS | __GFP_HARDWALL_BS)

/* Flag - indicates that the buffer will be suitable for DMA.  Ignored on some
   platforms, used as appropriate on others */

#define GFP_DMA_BS		__GFP_DMA_BS

/* 4GB DMA on some platforms */
#define GFP_DMA32_BS		__GFP_DMA32_BS

static inline int gfp_zone_bs(gfp_t_bs gfp)
{
	int zone = GFP_ZONEMASK_BS & (__force int) gfp;
	BUG_ON_BS(zone >= GFP_ZONETYPES_BS);
	return zone;
}

extern void __free_pages_bs(struct page_bs *page, unsigned int order);

#define __free_page_bs(page)	__free_pages_bs((page), 0)

extern void FASTCALL_BS(free_pages_bs(unsigned long addr, unsigned int order));

extern void arch_free_page_bs(struct page_bs *page, int order);

extern struct page_bs *
FASTCALL_BS(__alloc_pages_bs(gfp_t, unsigned int, struct zonelist_bs *));

static inline struct page_bs *alloc_pages_node_bs(int nid,
			gfp_t __nocast gfp_mask, unsigned int order)
{
	if (unlikely(order >= MAX_ORDER_BS))
		return NULL;

	/* Unknow node is current node */
	if (nid < 0)
		nid = numa_node_id_bs();

	return __alloc_pages_bs(gfp_mask, order,
			NODE_DATA_BS(nid)->node_zonelists +
			gfp_zone_bs(gfp_mask));
}

#define alloc_pages_bs(gfp_mask, order)	\
		alloc_pages_node_bs(numa_node_id_bs(), gfp_mask, order)
#define alloc_page_bs(gfp_mask)	alloc_pages_bs(gfp_mask, 0)

extern unsigned long
FASTCALL_BS(__get_free_pages_bs(gfp_t __nocast gfp_mask, unsigned int order));
extern unsigned long
FASTCALL_BS(get_zeroed_page_bs(gfp_t __nocast gfp_mask));

#define __get_free_page_bs(gfp_mask)				\
		__get_free_pages_bs((gfp_mask), 0)

#define free_page_bs(addr)	free_pages_bs((addr), 0)
#endif
