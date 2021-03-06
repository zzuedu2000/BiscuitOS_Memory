/*
 * arch/arm/mm/proc-v7-2level.S
 *
 * Copyright (C) 2001 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <asm/assembler.h>
#include "asm-generated/pgtable.h"

/*
 * cpu_v7_set_pte_ext_bs(ptep, pte, ext)
 *
 * Set a level 2 translation table entry.
 *
 * - ptep - pointer to level 2 translation table entry
 * - pte  - PTE value to store
 * - ext  - value for extended PTE bits.
 */
ENTRY(cpu_v7_set_pte_ext_bs)
#ifdef CONFIG_MMU
	str     r1, [r0]                        @ linux version

	bic	r3, r1, #0x000003f0
	bic	r3, r3, #PTE_TYPE_MASK_BS
	orr	r3, r3, r2
	orr	r3, r3, #PTE_EXT_AP0_BS | 2

	tst	r1, #1 << 4
	orrne	r3, r3, #PTE_EXT_TEX_BS(1)

	eor	r1, r1, #L_PTE_DIRTY_BS
	tst	r1, #L_PTE_RDONLY_BS | L_PTE_DIRTY_BS
	orrne	r3, r3, #PTE_EXT_APX_BS

	tst	r1, #L_PTE_USER_BS
	orrne	r3, r3, #PTE_EXT_AP1_BS

	tst	r1, #L_PTE_XN_BS
	orrne	r3, r3, #PTE_EXT_XN_BS

	tst	r1, #L_PTE_YOUNG_BS
	tstne	r1, #L_PTE_VALID_BS
	eorne	r1, r1, #L_PTE_NONE_BS
	tstne	r1, #L_PTE_NONE_BS
	moveq	r3, #0

 ARM(	str	r3, [r0, #2048]!	)
	ALT_SMP(W(nop))
	ALT_UP (mcr	p15, 0, r0, c7, c10, 1)		@ flush_pte
#endif
	bx	lr
ENDPROC(cpu_v7_set_pte_ext_bs)
