/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_020 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 33 */
/* NLOC: 126 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/ctype.h>
 * #include <linux/edac.h>
 * #include <linux/interrupt.h>
 * #include <linux/mfd/syscon.h>
 * #include <linux/module.h>
 * #include <linux/of.h>
 * #include <linux/of_address.h>
 * #include <linux/regmap.h>
 * #include <linux/string_choices.h>
 * #include "edac_module.h"
 */
/* Context-Before
 * #define CPUX_L2C_L2RTOALR_PAGE_OFFSET		0x0018
 * #define CPUX_L2C_L2RTOAHR_PAGE_OFFSET		0x001c
 * #define MEMERR_L2C_L2ESRA_PAGE_OFFSET		0x0804
 * 
 * /*
 *  * Processor Module Domain (PMD) context - Context for a pair of processors.
 *  * Each PMD consists of 2 CPUs and a shared L2 cache. Each CPU consists of
 *  * its own L1 cache.
 *  */
 * struct xgene_edac_pmd_ctx {
 * 	struct list_head	next;
 * 	struct device		ddev;
 * 	char			*name;
 * 	struct xgene_edac	*edac;
 * 	struct edac_device_ctl_info *edac_dev;
 * 	void __iomem		*pmd_csr;
 * 	u32			pmd;
 * 	int			version;
 * };
 */
static void xgene_edac_pmd_l1_check(struct edac_device_ctl_info *edac_dev,
				    int cpu_idx)
{
	struct xgene_edac_pmd_ctx *ctx = edac_dev->pvt_info;
	void __iomem *pg_f;
	u32 val;

	pg_f = ctx->pmd_csr + cpu_idx * CPU_CSR_STRIDE + CPU_MEMERR_CPU_PAGE;

	val = readl(pg_f + MEMERR_CPU_ICFESR_PAGE_OFFSET);
	if (!val)
		goto chk_lsu;
	dev_err(edac_dev->dev,
		"CPU%d L1 memory error ICF 0x%08X Way 0x%02X Index 0x%02X Info 0x%02X\n",
		ctx->pmd * MAX_CPU_PER_PMD + cpu_idx, val,
		MEMERR_CPU_ICFESR_ERRWAY_RD(val),
		MEMERR_CPU_ICFESR_ERRINDEX_RD(val),
		MEMERR_CPU_ICFESR_ERRINFO_RD(val));
	if (val & MEMERR_CPU_ICFESR_CERR_MASK)
		dev_err(edac_dev->dev, "One or more correctable error\n");
	if (val & MEMERR_CPU_ICFESR_MULTCERR_MASK)
		dev_err(edac_dev->dev, "Multiple correctable error\n");
	switch (MEMERR_CPU_ICFESR_ERRTYPE_RD(val)) {
	case 1:
		dev_err(edac_dev->dev, "L1 TLB multiple hit\n");
		break;
	case 2:
		dev_err(edac_dev->dev, "Way select multiple hit\n");
		break;
	case 3:
		dev_err(edac_dev->dev, "Physical tag parity error\n");
		break;
	case 4:
	case 5:
		dev_err(edac_dev->dev, "L1 data parity error\n");
		break;
	case 6:
		dev_err(edac_dev->dev, "L1 pre-decode parity error\n");
		break;
	}

	/* Clear any HW errors */
	writel(val, pg_f + MEMERR_CPU_ICFESR_PAGE_OFFSET);

	if (val & (MEMERR_CPU_ICFESR_CERR_MASK |
		   MEMERR_CPU_ICFESR_MULTCERR_MASK))
		edac_device_handle_ce(edac_dev, 0, 0, edac_dev->ctl_name);

chk_lsu:
	val = readl(pg_f + MEMERR_CPU_LSUESR_PAGE_OFFSET);
	if (!val)
		goto chk_mmu;
	dev_err(edac_dev->dev,
		"CPU%d memory error LSU 0x%08X Way 0x%02X Index 0x%02X Info 0x%02X\n",
		ctx->pmd * MAX_CPU_PER_PMD + cpu_idx, val,
		MEMERR_CPU_LSUESR_ERRWAY_RD(val),
		MEMERR_CPU_LSUESR_ERRINDEX_RD(val),
		MEMERR_CPU_LSUESR_ERRINFO_RD(val));
	if (val & MEMERR_CPU_LSUESR_CERR_MASK)
		dev_err(edac_dev->dev, "One or more correctable error\n");
	if (val & MEMERR_CPU_LSUESR_MULTCERR_MASK)
		dev_err(edac_dev->dev, "Multiple correctable error\n");
	switch (MEMERR_CPU_LSUESR_ERRTYPE_RD(val)) {
	case 0:
		dev_err(edac_dev->dev, "Load tag error\n");
		break;
	case 1:
		dev_err(edac_dev->dev, "Load data error\n");
		break;
	case 2:
		dev_err(edac_dev->dev, "WSL multihit error\n");
		break;
	case 3:
		dev_err(edac_dev->dev, "Store tag error\n");
		break;
	case 4:
		dev_err(edac_dev->dev,
			"DTB multihit from load pipeline error\n");
		break;
	case 5:
		dev_err(edac_dev->dev,
			"DTB multihit from store pipeline error\n");
		break;
	}

	/* Clear any HW errors */
	writel(val, pg_f + MEMERR_CPU_LSUESR_PAGE_OFFSET);

	if (val & (MEMERR_CPU_LSUESR_CERR_MASK |
		   MEMERR_CPU_LSUESR_MULTCERR_MASK))
		edac_device_handle_ce(edac_dev, 0, 0, edac_dev->ctl_name);

chk_mmu:
	val = readl(pg_f + MEMERR_CPU_MMUESR_PAGE_OFFSET);
	if (!val)
		return;
	dev_err(edac_dev->dev,
		"CPU%d memory error MMU 0x%08X Way 0x%02X Index 0x%02X Info 0x%02X %s\n",
		ctx->pmd * MAX_CPU_PER_PMD + cpu_idx, val,
		MEMERR_CPU_MMUESR_ERRWAY_RD(val),
		MEMERR_CPU_MMUESR_ERRINDEX_RD(val),
		MEMERR_CPU_MMUESR_ERRINFO_RD(val),
		val & MEMERR_CPU_MMUESR_ERRREQSTR_LSU_MASK ? "LSU" : "ICF");
	if (val & MEMERR_CPU_MMUESR_CERR_MASK)
		dev_err(edac_dev->dev, "One or more correctable error\n");
	if (val & MEMERR_CPU_MMUESR_MULTCERR_MASK)
		dev_err(edac_dev->dev, "Multiple correctable error\n");
	switch (MEMERR_CPU_MMUESR_ERRTYPE_RD(val)) {
	case 0:
		dev_err(edac_dev->dev, "Stage 1 UTB hit error\n");
		break;
	case 1:
		dev_err(edac_dev->dev, "Stage 1 UTB miss error\n");
		break;
	case 2:
		dev_err(edac_dev->dev, "Stage 1 UTB allocate error\n");
		break;
	case 3:
		dev_err(edac_dev->dev, "TMO operation single bank error\n");
		break;
	case 4:
		dev_err(edac_dev->dev, "Stage 2 UTB error\n");
		break;
	case 5:
		dev_err(edac_dev->dev, "Stage 2 UTB miss error\n");
		break;
	case 6:
		dev_err(edac_dev->dev, "Stage 2 UTB allocate error\n");
		break;
	case 7:
		dev_err(edac_dev->dev, "TMO operation multiple bank error\n");
		break;
	}

	/* Clear any HW errors */
	writel(val, pg_f + MEMERR_CPU_MMUESR_PAGE_OFFSET);

	edac_device_handle_ce(edac_dev, 0, 0, edac_dev->ctl_name);
}
/* Context-After
 * static void xgene_edac_pmd_l2_check(struct edac_device_ctl_info *edac_dev)
 * {
 * 	struct xgene_edac_pmd_ctx *ctx = edac_dev->pvt_info;
 * 	void __iomem *pg_d;
 * 	void __iomem *pg_e;
 * 	u32 val_hi;
 * 	u32 val_lo;
 * 	u32 val;
 * 
 * 	/* Check L2 */
 * 	pg_e = ctx->pmd_csr + CPU_MEMERR_L2C_PAGE;
 * 	val = readl(pg_e + MEMERR_L2C_L2ESR_PAGE_OFFSET);
 * 	if (!val)
 * 		goto chk_l2c;
 * 	val_lo = readl(pg_e + MEMERR_L2C_L2EALR_PAGE_OFFSET);
 * 	val_hi = readl(pg_e + MEMERR_L2C_L2EAHR_PAGE_OFFSET);
 * 	dev_err(edac_dev->dev,
 * 		"PMD%d memory error L2C L2ESR 0x%08X @ 0x%08X.%08X\n",
 * 		ctx->pmd, val, val_hi, val_lo);
 */
