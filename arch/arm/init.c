/*
 * arm/mm/init
 *
 * (C) 2020.04.14 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <asm/string.h>

#include "biscuitos/kernel.h"
#include "biscuitos/mm.h"
#include "biscuitos/bootmem.h"
#include "biscuitos/nodemask.h"
#include "biscuitos/mmzone.h"
#include "biscuitos/swap.h"
#include "biscuitos/mman.h"
#include "biscuitos/init.h"
#include "asm-generated/setup.h"
#include "asm-generated/arch.h"
#include "asm-generated/memory.h"
#include "asm-generated/map.h"
#include "asm-generated/tlbflush.h"
#include "asm-generated/string.h"
#include "asm-generated/cacheflush.h"

/* BiscuitOS Emulate */
extern unsigned long _stext_bs, _etext_bs, __init_begin_bs, _text_bs;
extern unsigned long _end_bs, __init_end_bs, __data_start_bs;

unsigned long phys_initrd_start_bs __initdata = 0;
unsigned long phys_initrd_size_bs __initdata = 0;
unsigned long initrd_start_bs, initrd_end_bs;
extern struct meminfo highmeminfo_bs;
extern void __init kmap_init_bs(void);

/*
 * empty_zero_page is a special page that is used for
 * zero-initialized data and COW.
 */
struct page_bs *empty_zero_page_bs;

/*
 * The sole use of this is to pass memory configuration
 * data from paging_init to mem_init
 */
static struct meminfo meminfo_bs __initdata = { 0, };

struct node_info {
	unsigned int start;
	unsigned int end;
	int bootmap_pages;
};

#define O_PFN_DOWN(x)	((x) >> PAGE_SHIFT_BS)
#define V_PFN_DOWN(x)	O_PFN_DOWN(__pa_bs(x))

#define O_PFN_UP(x)	(PAGE_ALIGN_BS(x) >> PAGE_SHIFT_BS)
#define V_PFN_UP(x)	O_PFN_UP(__pa_bs(x))

/*
 * Scan the memory info structure and pull out:
 *  - the end of memory
 *  - the number of nodes
 *  - the pfn range of each node
 *  - the number of bootmem bitmap pages
 */
static unsigned int __init
find_memend_and_nodes_bs(struct meminfo *mi, struct node_info *np)
{
	unsigned int i, bootmem_pages = 0, memend_pfn = 0;

	for (i = 0; i < MAX_NUMNODES_BS; i++) {
		np[i].start = -1U;
		np[i].end = 0;
		np[i].bootmap_pages = 0;
	}

	for (i = 0; i < mi->nr_banks; i++) {
		unsigned long start, end;
		int node;

		if (mi->bank[i].size == 0) {
			/*
			 * Mark this bank with an invalid node number
			 */
			mi->bank[i].node = -1;
			continue;
		}

		node = mi->bank[i].node;

		/*
		 * Make sure we haven't exceeded the maximum number of nodes
		 * that we have in this configuration. If we have, we're in
		 * trouble. (maybe we ought to limit, instead of bugging?)
		 */
		if (node >= MAX_NUMNODES_BS)
			BUG_BS();
		node_set_online_bs(node);

		/*
		 * Get the start and end pfns for this bank
		 */
		start = O_PFN_UP(mi->bank[i].start);
		end   = O_PFN_DOWN(mi->bank[i].start + mi->bank[i].size);

		if (np[node].start > start)
			np[node].start = start;

		if (np[node].end < end)
			np[node].end = end;

		if (memend_pfn < end)
			memend_pfn = end;
	}

#ifdef CONFIG_HIGHMEM_BS
	/* FIXME: Default ARM doesn't support HighMem Zone,
	 * BiscuitOS support HighMem Zone.
	 */
	np[0].end += O_PFN_DOWN(highmeminfo_bs.bank[0].size);
#endif

	/*
	 * Calculate the number of pages we requires to
	 * store the bootmem bitmap.
	 */
	for_each_online_node_bs(i) {
		if (np[i].end == 0)
			continue;
		np[i].bootmap_pages = bootmem_bootmap_pages_bs(np[i].end -
							np[i].start);
		bootmem_pages += np[i].bootmap_pages;
	}

	high_memory_bs = __va_bs(memend_pfn << PAGE_SHIFT_BS);

	/*
	 * This doesn't seem to be used by the Linux memory
	 * manager any more. If we can get rid of it, we
	 * also get rid of some of the stuff above as well.
	 *
	 * Note: max_low_pfn and max_pfn reflect the number
	 * of _pages_in the system, not the maximum PFN.
	 */
	max_low_pfn_bs = memend_pfn - O_PFN_DOWN(PHYS_OFFSET_BS);
	max_pfn_bs = memend_pfn - O_PFN_DOWN(PHYS_OFFSET_BS);

	return bootmem_pages;
}

/*
 * FIXME: We really want to avoid allocating the bootmap bitmap
 * over the top of the initrd. Hopefully, this is located towards
 * the start of a bank, so if we allocate the bootmap bitmap at
 * the end, we won't clash.
 */
static unsigned int __init
find_bootmap_pfn_bs(int node, struct meminfo *mi, unsigned int bootmap_pages)
{
	unsigned int start_pfn, bank, bootmap_pfn;

	start_pfn   = V_PFN_UP(_end_bs);
	bootmap_pfn = 0;

	for (bank = 0; bank < mi->nr_banks; bank++) {
		unsigned long start, end;

		if (mi->bank[bank].node != node)
			continue;

		start = O_PFN_UP(mi->bank[bank].start);
		end   = O_PFN_DOWN(mi->bank[bank].size +
					mi->bank[bank].start);

		if (end < start_pfn)
			continue;

		if (start < start_pfn)
			start = start_pfn;

		if (end <= start)
			continue;

		if (end - start >= bootmap_pages) {
			bootmap_pfn = start;
			break;
		}
	}

	if (bootmap_pfn == 0)
		BUG_BS();

	return bootmap_pfn;
}

static int __init check_initrd_bs(struct meminfo *mi)
{
	int initrd_node = -2;
	unsigned long end = phys_initrd_start_bs + phys_initrd_size_bs;

	/*
	 * Make sure that the initrd is within a valid area of
	 * memory.
	 */
	if (phys_initrd_size_bs) {
		unsigned int i;

		initrd_node = -1;

		for (i = 0; i < mi->nr_banks; i++) {
			unsigned long bank_end;

			bank_end = mi->bank[i].start + mi->bank[i].size;

			if (mi->bank[i].start <= phys_initrd_start_bs &&
					end <= bank_end)
				initrd_node = mi->bank[i].node;
		}
	}

	if (initrd_node == -1) {
		printk(KERN_ERR "initrd (0x%08lx - 0x%08lx) extends beyond "
			"physical memory - disabling initrd\n",
				phys_initrd_start_bs, end);
		phys_initrd_start_bs = phys_initrd_size_bs = 0;
	}

	return initrd_node;
}

/*
 * Register all available RAM in this node with the bootmem allocator.
 */
static inline void free_bootmem_node_bank_bs(int node, struct meminfo *mi)
{
	pg_data_t_bs *pgdat = NODE_DATA_BS(node);
	int bank;

	for (bank = 0; bank < mi->nr_banks; bank++)
		if (mi->bank[bank].node == node)
			free_bootmem_node_bs(pgdat, mi->bank[bank].start,
				mi->bank[bank].size);

#ifdef CONFIG_HIGHMEM_BS
	/* FIXME: Default ARM doesn't support HighMem Zone,
	 * BiscuitOS support HighMem Zone
	 */
	free_bootmem_node_bs(pgdat, highmeminfo_bs.bank[0].start,
					highmeminfo_bs.bank[0].size);
#endif
}

/*
 * Reserve the various regions of node 0
 */
static __init void reserve_node_zero_bs(unsigned int bootmap_pfn, 
						unsigned int bootmap_pages)
{
	pg_data_t_bs *pgdat = NODE_DATA_BS(0);

	/*
	 * Register the kernel text and data with bootmeme.
	 * Note that this can only be in node 0.
	 */
	reserve_bootmem_node_bs(pgdat, __pa_bs(_stext_bs), 
							_end_bs - _stext_bs);

	/*
	 * Reserve the page table. These are already in use,
	 * and can only be in node 0.
	 */
	reserve_bootmem_node_bs(pgdat, __pa_bs(swapper_pg_dir_bs),
			PTRS_PER_PGD_BS * sizeof(pgd_t_bs));

	/*
	 * And don't forget to reserve the allocator bitmap,
	 * which will be freed later.
	 */
	reserve_bootmem_node_bs(pgdat, bootmap_pfn << PAGE_SHIFT_BS,
			bootmap_pages << PAGE_SHIFT_BS);
}

/* FIXME: BiscuitOS bootmem entry */
DEBUG_FUNC_T(bootmem);

/*
 * Initialise the bootmem allocator for all nodes. This is called
 * early during the architecture specific initialisation.
 */
static void __init bootmem_init_bs(struct meminfo *mi)
{
	struct node_info node_info[MAX_NUMNODES_BS], *np = node_info;
	unsigned int bootmap_pages, bootmap_pfn, map_pg;
	int node, initrd_node;

	bootmap_pages = find_memend_and_nodes_bs(mi, np);
	bootmap_pfn   = find_bootmap_pfn_bs(0, mi, bootmap_pages);
	initrd_node   = check_initrd_bs(mi);

	map_pg = bootmap_pfn;

	/*
	 * Initialise the bootmem nodes.
	 *
	 * What we really want to do is:
	 *
	 *   unmap_all_regions_except_kernel();
	 *   for_each_node_in_reverse_order(node) {
	 *   	map_node(node);
	 *   	allocate_bootmap_map(node);
	 *   	init_bootmem_node(node);
	 *   	free_bootmem_node(node);
	 *   }
	 *
	 * but this is a 2.5-type change. For now, we just set
	 * the nodes up in reverse order.
	 *
	 * (we could also do with rolling bootmem_init and paging_init
	 * into one generic "memory_init" type function).
	 */
	np += num_online_nodes_bs() - 1;
	for (node = num_online_nodes_bs() - 1; node >= 0; node--, np--) {
		/*
		 * If there are no pages in this node, ignore it.
		 * Note that node 0 must always have some pages.
		 */
		if (np->end == 0 || !node_online_bs(node)) {
			if (node == 0)
				BUG_BS();
			continue;
		}

		/*
		 * Initialise the bootmem allocator.
		 */
		init_bootmem_node_bs(NODE_DATA_BS(node), map_pg,
							np->start, np->end);
		free_bootmem_node_bank_bs(node, mi);
		map_pg += np->bootmap_pages;

		/*
		 * If this is node 0, we need to reserve some areas ASAP -
		 * we may use bootmem on node 0 to setup the other nodes.
		 */
		if (node == 0)
			reserve_node_zero_bs(bootmap_pfn, bootmap_pages);
	}

	if (phys_initrd_size_bs && initrd_node >= 0) {
		reserve_bootmem_node_bs(NODE_DATA_BS(initrd_node),
				phys_initrd_start_bs, phys_initrd_size_bs);
		initrd_start_bs = __phys_to_virt_bs(phys_initrd_start_bs);
		initrd_end_bs = initrd_start_bs + phys_initrd_size_bs;
	}

	BUG_ON_BS(map_pg != bootmap_pfn + bootmap_pages);

	/* FIXME: bootmem_initcall entry, used to debug bootmem,
	 * This code isn't default code */
	DEBUG_CALL(bootmem);
}

/*
 * paging_init() sets up the page tables, initializes the zone memory
 * maps, and sets up the zero page, bad page and page table.
 */
void __init paging_init_bs(struct meminfo *mi, struct machine_desc_bs *mdesc)
{
	void *zero_page;
	int node;

	bootmem_init_bs(mi);

	memcpy(&meminfo_bs, mi, sizeof(meminfo_bs));

	/*
	 * allocate the zero page. Note that we count on this going ok.
	 */
	zero_page = alloc_bootmem_low_pages_bs(PAGE_SIZE_BS);

	/*
	 * initialise the page tables.
	 */
	memtable_init_bs(mi);
	if (mdesc->map_io)
		mdesc->map_io();

	flush_tlb_all_bs();

	/*
	 * initialize the zones within each node
	 */
	for_each_online_node_bs(node) {
		unsigned long zone_size[MAX_NR_ZONES_BS];
		unsigned long zhole_size[MAX_NR_ZONES_BS];
		struct bootmem_data_bs *bdata;
		pg_data_t_bs *pgdat;
		int i;

		/*
		 * Initialise the zone size information
		 */
		for (i = 0; i < MAX_NR_ZONES_BS; i++) {
			zone_size[i] = 0;
			zhole_size[i] = 0;
		}

		pgdat = NODE_DATA_BS(node);
		bdata = pgdat->bdata;

		/*
		 * The size of this node has already been determined.
		 * If we need to do anything fancy with the allocation
		 * of this memory to the zones, now is the time to do
		 * it.
		 */
		zone_size[0] = bdata->node_low_pfn -
				(bdata->node_boot_start >> PAGE_SHIFT_BS);

		/*
		 * If this zone has zero size, skip it.
		 */
		if (!zone_size[0])
			continue;

		/*
		 * For each bank in this node, calculate the size of the
		 * holes.  holes = node_size - sum(bank_sizes_in_node)
		 */
		zhole_size[0] = zone_size[0];
		for (i = 0; i < mi->nr_banks; i++) {
			if (mi->bank[i].node != node)
				continue;

			zhole_size[0] -= mi->bank[i].size >> PAGE_SHIFT_BS;
		}

		/*
		 * Adjust the sizes according to any special
		 * requirements for this machine type.
		 */
		arch_adjust_zones_bs(node, zone_size, zhole_size);

		free_area_init_node_bs(node, pgdat, zone_size,
			bdata->node_boot_start >> PAGE_SHIFT_BS, zhole_size);
	}

	/*
	 * finish off the bad pages once the mem_map is initialized
	 */
	memzero_bs(zero_page, PAGE_SIZE);
	empty_zero_page_bs = virt_to_page_bs(zero_page);
	/* FIXME: ignore? */
	flush_dcache_page_bs(empty_zero_page_bs);

	/* FIXME: Kmap from Intel-i386 and high-version ARM */
	kmap_init_bs();
}

/* FIXME: BiscuitOS buddy debug stuf */
DEBUG_FUNC_T(buddy);
DEBUG_FUNC_T(pcp);

/*
 * mem_init() marks the free areas in the mem_map and tells us how much
 * memory is free. This is done after various parts of the system have
 * claimed their memory after the kernel image.
 */
void __init mem_init_bs(void)
{
	unsigned int codepages, datapages, initpages;
	int i, node;

	codepages = _etext_bs - _text_bs;
	datapages = _end_bs - __data_start_bs;
	initpages = __init_end_bs - __init_begin_bs;

	max_mapnr_bs = virt_to_page_bs(high_memory_bs) - mem_map_bs;

	/*
	 * We may have non-contiguous memory
	 */
	if (meminfo_bs.nr_banks != 1)
		create_memmap_holes_bs(&meminfo_bs);

	/* this will put all unused low memory onto the freelists */
	for_each_online_node_bs(node) {
		pg_data_t_bs *pgdat = NODE_DATA_BS(node);

		if (pgdat->node_spanned_pages != 0)
			totalram_pages_bs += free_all_bootmem_node_bs(pgdat);
	}

	/*
	 * Since our memory may not be contiguous, calculate the
	 * real number of pages we have in this system
	 */
	printk(KERN_INFO "Memory:");

	num_physpages_bs = 0;
	for (i = 0; i < meminfo_bs.nr_banks; i++) {
		num_physpages_bs +=
				meminfo_bs.bank[i].size >> PAGE_SHIFT_BS;
		printk(" %ldMB", meminfo_bs.bank[i].size >> 20);
	}
#ifdef CONFIG_HIGHMEM_BS
	for (i = 0; i < highmeminfo_bs.nr_banks; i++) {
		num_physpages_bs += 
				highmeminfo_bs.bank[i].size >> PAGE_SHIFT_BS;
		printk(" %ldMB", highmeminfo_bs.bank[i].size >> 20);
	}
#endif

	printk(" = %luMB total\n", num_physpages_bs >> (20 - PAGE_SHIFT_BS));
	printk(KERN_NOTICE "Memory: %luKB available (%dK code, "
			"%dK data, %dK init)\n",
		(unsigned long)nr_free_pages_bs() << (PAGE_SHIFT_BS-10),
		codepages >> 10, datapages >> 10, initpages >> 10);

	if (PAGE_SIZE_BS > 16364 && num_physpages_bs <= 128) {
		extern int sysctl_overcommit_memory;

		/*
		 * On a machine this small we won't get
		 * anywhere without overcommit, so turn
		 * it on by default.
		*/
		sysctl_overcommit_memory = OVERCOMMIT_ALWAYS_BS;

	}

	/* FIXME: buddy_initcall entry, used to debug buddy,
	 * This code isn't default code */
	DEBUG_CALL(buddy);
	DEBUG_CALL(pcp);
}
