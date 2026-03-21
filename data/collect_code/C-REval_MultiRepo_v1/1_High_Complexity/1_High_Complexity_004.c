/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_004 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 35 */
/* NLOC: 112 */
/* Subsystem: arch */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/kcore.h>
 * #include <linux/types.h>
 * #include <linux/init.h>
 * #include <linux/memblock.h>
 * #include <linux/mmzone.h>
 * #include <linux/pci_ids.h>
 * #include <linux/pci.h>
 * #include <linux/bitops.h>
 * #include <linux/suspend.h>
 * #include <asm/e820/api.h>
 * #include <asm/io.h>
 * #include <asm/iommu.h>
 * #include <asm/gart.h>
 * #include <asm/pci-direct.h>
 * #include <asm/dma.h>
 * #include <asm/amd/nb.h>
 * #include <asm/x86_init.h>
 * #include <linux/crash_dump.h>
 */
/* Context-Before
 * 		int dev_base, dev_limit;
 * 
 * 		bus = amd_nb_bus_dev_ranges[i].bus;
 * 		dev_base = amd_nb_bus_dev_ranges[i].dev_base;
 * 		dev_limit = amd_nb_bus_dev_ranges[i].dev_limit;
 * 
 * 		for (slot = dev_base; slot < dev_limit; slot++) {
 * 			if (!early_is_amd_nb(read_pci_config(bus, slot, 3, 0x00)))
 * 				continue;
 * 
 * 			ctl = read_pci_config(bus, slot, 3, AMD64_GARTAPERTURECTL);
 * 			ctl &= ~GARTEN;
 * 			write_pci_config(bus, slot, 3, AMD64_GARTAPERTURECTL, ctl);
 * 		}
 * 	}
 * 
 * }
 * 
 * static int __initdata printed_gart_size_msg;
 */
void __init gart_iommu_hole_init(void)
{
	u32 agp_aper_base = 0, agp_aper_order = 0;
	u32 aper_size, aper_alloc = 0, aper_order = 0, last_aper_order = 0;
	u64 aper_base, last_aper_base = 0;
	int fix, slot, valid_agp = 0;
	int i, node;

	if (!amd_gart_present())
		return;

	if (gart_iommu_aperture_disabled || !fix_aperture ||
	    !early_pci_allowed())
		return;

	pr_info("Checking aperture...\n");

	if (!fallback_aper_force)
		agp_aper_base = search_agp_bridge(&agp_aper_order, &valid_agp);

	fix = 0;
	node = 0;
	for (i = 0; i < amd_nb_bus_dev_ranges[i].dev_limit; i++) {
		int bus;
		int dev_base, dev_limit;
		u32 ctl;

		bus = amd_nb_bus_dev_ranges[i].bus;
		dev_base = amd_nb_bus_dev_ranges[i].dev_base;
		dev_limit = amd_nb_bus_dev_ranges[i].dev_limit;

		for (slot = dev_base; slot < dev_limit; slot++) {
			if (!early_is_amd_nb(read_pci_config(bus, slot, 3, 0x00)))
				continue;

			iommu_detected = 1;
			gart_iommu_aperture = 1;
			x86_init.iommu.iommu_init = gart_iommu_init;

			ctl = read_pci_config(bus, slot, 3,
					      AMD64_GARTAPERTURECTL);

			/*
			 * Before we do anything else disable the GART. It may
			 * still be enabled if we boot into a crash-kernel here.
			 * Reconfiguring the GART while it is enabled could have
			 * unknown side-effects.
			 */
			ctl &= ~GARTEN;
			write_pci_config(bus, slot, 3, AMD64_GARTAPERTURECTL, ctl);

			aper_order = (ctl >> 1) & 7;
			aper_size = (32 * 1024 * 1024) << aper_order;
			aper_base = read_pci_config(bus, slot, 3, AMD64_GARTAPERTUREBASE) & 0x7fff;
			aper_base <<= 25;

			pr_info("Node %d: aperture [bus addr %#010Lx-%#010Lx] (%uMB)\n",
				node, aper_base, aper_base + aper_size - 1,
				aper_size >> 20);
			node++;

			if (!aperture_valid(aper_base, aper_size, 64<<20)) {
				if (valid_agp && agp_aper_base &&
				    agp_aper_base == aper_base &&
				    agp_aper_order == aper_order) {
					/* the same between two setting from NB and agp */
					if (!no_iommu &&
					    max_pfn > MAX_DMA32_PFN &&
					    !printed_gart_size_msg) {
						pr_err("you are using iommu with agp, but GART size is less than 64MB\n");
						pr_err("please increase GART size in your BIOS setup\n");
						pr_err("if BIOS doesn't have that option, contact your HW vendor!\n");
						printed_gart_size_msg = 1;
					}
				} else {
					fix = 1;
					goto out;
				}
			}

			if ((last_aper_order && aper_order != last_aper_order) ||
			    (last_aper_base && aper_base != last_aper_base)) {
				fix = 1;
				goto out;
			}
			last_aper_order = aper_order;
			last_aper_base = aper_base;
		}
	}

out:
	if (!fix && !fallback_aper_force) {
		if (last_aper_base) {
			/*
			 * If this is the kdump kernel, the first kernel
			 * may have allocated the range over its e820 RAM
			 * and fixed up the northbridge
			 */
			exclude_from_core(last_aper_base, last_aper_order);
		}
		return;
	}

	if (!fallback_aper_force) {
		aper_alloc = agp_aper_base;
		aper_order = agp_aper_order;
	}

	if (aper_alloc) {
		/* Got the aperture from the AGP bridge */
	} else if ((!no_iommu && max_pfn > MAX_DMA32_PFN) ||
		   force_iommu ||
		   valid_agp ||
		   fallback_aper_force) {
		pr_info("Your BIOS doesn't leave an aperture memory hole\n");
		pr_info("Please enable the IOMMU option in the BIOS setup\n");
		pr_info("This costs you %dMB of RAM\n",
			32 << fallback_aper_order);

		aper_order = fallback_aper_order;
		aper_alloc = allocate_aperture();
		if (!aper_alloc) {
			/*
			 * Could disable AGP and IOMMU here, but it's
			 * probably not worth it. But the later users
			 * cannot deal with bad apertures and turning
			 * on the aperture over memory causes very
			 * strange problems, so it's better to panic
			 * early.
			 */
			panic("Not enough memory for aperture");
		}
	} else {
		return;
	}

	/*
	 * If this is the kdump kernel _and_ the first kernel did not
	 * configure the aperture in the northbridge, this range may
	 * overlap with the first kernel's memory. We can't access the
	 * range through vmcore even though it should be part of the dump.
	 */
	exclude_from_core(aper_alloc, aper_order);

	/* Fix up the north bridges */
	for (i = 0; i < amd_nb_bus_dev_ranges[i].dev_limit; i++) {
		int bus, dev_base, dev_limit;

		/*
		 * Don't enable translation yet but enable GART IO and CPU
		 * accesses and set DISTLBWALKPRB since GART table memory is UC.
		 */
		u32 ctl = aper_order << 1;

		bus = amd_nb_bus_dev_ranges[i].bus;
		dev_base = amd_nb_bus_dev_ranges[i].dev_base;
		dev_limit = amd_nb_bus_dev_ranges[i].dev_limit;
		for (slot = dev_base; slot < dev_limit; slot++) {
			if (!early_is_amd_nb(read_pci_config(bus, slot, 3, 0x00)))
				continue;

			write_pci_config(bus, slot, 3, AMD64_GARTAPERTURECTL, ctl);
			write_pci_config(bus, slot, 3, AMD64_GARTAPERTUREBASE, aper_alloc >> 25);
		}
	}

	set_up_gart_resume(aper_order, aper_alloc);
}
/* Context-After: <empty> */
