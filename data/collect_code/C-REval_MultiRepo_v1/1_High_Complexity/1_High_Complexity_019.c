/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_019 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 38 */
/* NLOC: 125 */
/* Subsystem: arch */
/* Includes
 * #include <linux/string.h>
 * #include <linux/perf_event.h>
 * #include <asm/reg.h>
 * #include <asm/cputable.h>
 * #include "internal.h"
 */
/* Context-Before
 * 		value |= (unsigned long)spcsel << 48;
 * 	}
 * 	*maskp = mask;
 * 	*valp = value;
 * 	return 0;
 * }
 * 
 * static int p970_get_alternatives(u64 event, unsigned int flags, u64 alt[])
 * {
 * 	alt[0] = event;
 * 
 * 	/* 2 alternatives for LSU empty */
 * 	if (event == 0x2002 || event == 0x3002) {
 * 		alt[1] = event ^ 0x1000;
 * 		return 2;
 * 	}
 * 
 * 	return 1;
 * }
 */
static int p970_compute_mmcr(u64 event[], int n_ev,
			     unsigned int hwc[], struct mmcr_regs *mmcr,
			     struct perf_event *pevents[],
			     u32 flags __maybe_unused)
{
	unsigned long mmcr0 = 0, mmcr1 = 0, mmcra = 0;
	unsigned int pmc, unit, byte, psel;
	unsigned int ttm, grp;
	unsigned int pmc_inuse = 0;
	unsigned int pmc_grp_use[2];
	unsigned char busbyte[4];
	unsigned char unituse[16];
	unsigned char unitmap[] = { 0, 0<<3, 3<<3, 1<<3, 2<<3, 0|4, 3|4 };
	unsigned char ttmuse[2];
	unsigned char pmcsel[8];
	int i;
	int spcsel;

	if (n_ev > 8)
		return -1;

	/* First pass to count resource use */
	pmc_grp_use[0] = pmc_grp_use[1] = 0;
	memset(busbyte, 0, sizeof(busbyte));
	memset(unituse, 0, sizeof(unituse));
	for (i = 0; i < n_ev; ++i) {
		pmc = (event[i] >> PM_PMC_SH) & PM_PMC_MSK;
		if (pmc) {
			if (pmc_inuse & (1 << (pmc - 1)))
				return -1;
			pmc_inuse |= 1 << (pmc - 1);
			/* count 1/2/5/6 vs 3/4/7/8 use */
			++pmc_grp_use[((pmc - 1) >> 1) & 1];
		}
		unit = (event[i] >> PM_UNIT_SH) & PM_UNIT_MSK;
		byte = (event[i] >> PM_BYTE_SH) & PM_BYTE_MSK;
		if (unit) {
			if (unit > PM_LASTUNIT)
				return -1;
			if (!pmc)
				++pmc_grp_use[byte & 1];
			if (busbyte[byte] && busbyte[byte] != unit)
				return -1;
			busbyte[byte] = unit;
			unituse[unit] = 1;
		}
	}
	if (pmc_grp_use[0] > 4 || pmc_grp_use[1] > 4)
		return -1;

	/*
	 * Assign resources and set multiplexer selects.
	 *
	 * PM_ISU can go either on TTM0 or TTM1, but that's the only
	 * choice we have to deal with.
	 */
	if (unituse[PM_ISU] &
	    (unituse[PM_FPU] | unituse[PM_IFU] | unituse[PM_VPU]))
		unitmap[PM_ISU] = 2 | 4;	/* move ISU to TTM1 */
	/* Set TTM[01]SEL fields. */
	ttmuse[0] = ttmuse[1] = 0;
	for (i = PM_FPU; i <= PM_STS; ++i) {
		if (!unituse[i])
			continue;
		ttm = unitmap[i];
		++ttmuse[(ttm >> 2) & 1];
		mmcr1 |= (unsigned long)(ttm & ~4) << MMCR1_TTM1SEL_SH;
	}
	/* Check only one unit per TTMx */
	if (ttmuse[0] > 1 || ttmuse[1] > 1)
		return -1;

	/* Set byte lane select fields and TTM3SEL. */
	for (byte = 0; byte < 4; ++byte) {
		unit = busbyte[byte];
		if (!unit)
			continue;
		if (unit <= PM_STS)
			ttm = (unitmap[unit] >> 2) & 1;
		else if (unit == PM_LSU0)
			ttm = 2;
		else {
			ttm = 3;
			if (unit == PM_LSU1L && byte >= 2)
				mmcr1 |= 1ull << (MMCR1_TTM3SEL_SH + 3 - byte);
		}
		mmcr1 |= (unsigned long)ttm
			<< (MMCR1_TD_CP_DBG0SEL_SH - 2 * byte);
	}

	/* Second pass: assign PMCs, set PMCxSEL and PMCx_ADDER_SEL fields */
	memset(pmcsel, 0x8, sizeof(pmcsel));	/* 8 means don't count */
	for (i = 0; i < n_ev; ++i) {
		pmc = (event[i] >> PM_PMC_SH) & PM_PMC_MSK;
		unit = (event[i] >> PM_UNIT_SH) & PM_UNIT_MSK;
		byte = (event[i] >> PM_BYTE_SH) & PM_BYTE_MSK;
		psel = event[i] & PM_PMCSEL_MSK;
		if (!pmc) {
			/* Bus event or any-PMC direct event */
			if (unit)
				psel |= 0x10 | ((byte & 2) << 2);
			else
				psel |= 8;
			for (pmc = 0; pmc < 8; ++pmc) {
				if (pmc_inuse & (1 << pmc))
					continue;
				grp = (pmc >> 1) & 1;
				if (unit) {
					if (grp == (byte & 1))
						break;
				} else if (pmc_grp_use[grp] < 4) {
					++pmc_grp_use[grp];
					break;
				}
			}
			pmc_inuse |= 1 << pmc;
		} else {
			/* Direct event */
			--pmc;
			if (psel == 0 && (byte & 2))
				/* add events on higher-numbered bus */
				mmcr1 |= 1ull << mmcr1_adder_bits[pmc];
		}
		pmcsel[pmc] = psel;
		hwc[i] = pmc;
		spcsel = (event[i] >> PM_SPCSEL_SH) & PM_SPCSEL_MSK;
		mmcr1 |= spcsel;
		if (p970_marked_instr_event(event[i]))
			mmcra |= MMCRA_SAMPLE_ENABLE;
	}
	for (pmc = 0; pmc < 2; ++pmc)
		mmcr0 |= pmcsel[pmc] << (MMCR0_PMC1SEL_SH - 7 * pmc);
	for (; pmc < 8; ++pmc)
		mmcr1 |= (unsigned long)pmcsel[pmc]
			<< (MMCR1_PMC3SEL_SH - 5 * (pmc - 2));
	if (pmc_inuse & 1)
		mmcr0 |= MMCR0_PMC1CE;
	if (pmc_inuse & 0xfe)
		mmcr0 |= MMCR0_PMCjCE;

	mmcra |= 0x2000;	/* mark only one IOP per PPC instruction */

	/* Return MMCRx values */
	mmcr->mmcr0 = mmcr0;
	mmcr->mmcr1 = mmcr1;
	mmcr->mmcra = mmcra;
	return 0;
}
/* Context-After
 * static void p970_disable_pmc(unsigned int pmc, struct mmcr_regs *mmcr)
 * {
 * 	int shift;
 * 
 * 	/*
 * 	 * Setting the PMCxSEL field to 0x08 disables PMC x.
 * 	 */
 * 	if (pmc <= 1) {
 * 		shift = MMCR0_PMC1SEL_SH - 7 * pmc;
 * 		mmcr->mmcr0 = (mmcr->mmcr0 & ~(0x1fUL << shift)) | (0x08UL << shift);
 * 	} else {
 * 		shift = MMCR1_PMC3SEL_SH - 5 * (pmc - 2);
 * 		mmcr->mmcr1 = (mmcr->mmcr1 & ~(0x1fUL << shift)) | (0x08UL << shift);
 * 	}
 * }
 * 
 * static int ppc970_generic_events[] = {
 * 	[PERF_COUNT_HW_CPU_CYCLES]		= 7,
 * 	[PERF_COUNT_HW_INSTRUCTIONS]		= 1,
 */
