/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_047 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 51 */
/* NLOC: 180 */
/* Subsystem: arch */
/* Includes
 * #include <linux/types.h>
 * #include <linux/mm.h>
 * #include <linux/seq_file.h>
 * #include <linux/console.h>
 * #include <linux/init.h>
 * #include <linux/delay.h>
 * #include <linux/ioport.h>
 * #include <linux/platform_device.h>
 * #include <linux/usb/isp116x.h>
 * #include <linux/module.h>
 * #include <asm/bootinfo.h>
 * #include <asm/bootinfo-atari.h>
 * #include <asm/byteorder.h>
 * #include <asm/setup.h>
 * #include <asm/atarihw.h>
 * #include <asm/atariints.h>
 * #include <asm/atari_stram.h>
 * #include <asm/machdep.h>
 * #include <asm/hwtest.h>
 * #include <asm/io.h>
 */
/* Context-Before
 * 			atari_switches |= ATARI_SWITCH_IKBD << ovsc_shift;
 * 		} else if (strcmp(p, "midi") == 0) {
 * 			/* RTS line of MIDI ACIA */
 * 			atari_switches |= ATARI_SWITCH_MIDI << ovsc_shift;
 * 		} else if (strcmp(p, "snd6") == 0) {
 * 			atari_switches |= ATARI_SWITCH_SND6 << ovsc_shift;
 * 		} else if (strcmp(p, "snd7") == 0) {
 * 			atari_switches |= ATARI_SWITCH_SND7 << ovsc_shift;
 * 		}
 * 	}
 * 	return 0;
 * }
 * 
 * early_param("switches", atari_switches_setup);
 * 
 * 
 *     /*
 *      *  Setup the Atari configuration info
 *      */
 */
void __init config_atari(void)
{
	unsigned short tos_version;

	memset(&atari_hw_present, 0, sizeof(atari_hw_present));

	/* Change size of I/O space from 64KB to 4GB. */
	ioport_resource.end  = 0xFFFFFFFF;

	mach_sched_init      = atari_sched_init;
	mach_init_IRQ        = atari_init_IRQ;
	mach_get_model	 = atari_get_model;
	mach_get_hardware_list = atari_get_hardware_list;
	mach_reset           = atari_reset;
#if IS_ENABLED(CONFIG_INPUT_M68K_BEEP)
	mach_beep          = atari_mksound;
#endif
#ifdef CONFIG_HEARTBEAT
	mach_heartbeat = atari_heartbeat;
#endif

	/* Set switches as requested by the user */
	if (atari_switches & ATARI_SWITCH_IKBD)
		acia.key_ctrl = ACIA_DIV64 | ACIA_D8N1S | ACIA_RHTID;
	if (atari_switches & ATARI_SWITCH_MIDI)
		acia.mid_ctrl = ACIA_DIV16 | ACIA_D8N1S | ACIA_RHTID;
	if (atari_switches & (ATARI_SWITCH_SND6|ATARI_SWITCH_SND7)) {
		sound_ym.rd_data_reg_sel = 14;
		sound_ym.wd_data = sound_ym.rd_data_reg_sel |
				   ((atari_switches&ATARI_SWITCH_SND6) ? 0x40 : 0) |
				   ((atari_switches&ATARI_SWITCH_SND7) ? 0x80 : 0);
	}

	/* ++bjoern:
	 * Determine hardware present
	 */

	pr_info("Atari hardware found:");
	if (MACH_IS_MEDUSA) {
		/* There's no Atari video hardware on the Medusa, but all the
		 * addresses below generate a DTACK so no bus error occurs! */
	} else if (hwreg_present(f030_xreg)) {
		ATARIHW_SET(VIDEL_SHIFTER);
		pr_cont(" VIDEL");
		/* This is a temporary hack: If there is Falcon video
		 * hardware, we assume that the ST-DMA serves SCSI instead of
		 * ACSI. In the future, there should be a better method for
		 * this...
		 */
		ATARIHW_SET(ST_SCSI);
		pr_cont(" STDMA-SCSI");
	} else if (hwreg_present(tt_palette)) {
		ATARIHW_SET(TT_SHIFTER);
		pr_cont(" TT_SHIFTER");
	} else if (hwreg_present(&shifter_st.bas_hi)) {
		if (hwreg_present(&shifter_st.bas_lo) &&
		    (shifter_st.bas_lo = 0x0aau, shifter_st.bas_lo == 0x0aau)) {
			ATARIHW_SET(EXTD_SHIFTER);
			pr_cont(" EXTD_SHIFTER");
		} else {
			ATARIHW_SET(STND_SHIFTER);
			pr_cont(" STND_SHIFTER");
		}
	}
	if (hwreg_present(&st_mfp.par_dt_reg)) {
		ATARIHW_SET(ST_MFP);
		pr_cont(" ST_MFP");
	}
	if (hwreg_present(&tt_mfp.par_dt_reg)) {
		ATARIHW_SET(TT_MFP);
		pr_cont(" TT_MFP");
	}
	if (hwreg_present(&tt_scsi_dma.dma_addr_hi)) {
		ATARIHW_SET(SCSI_DMA);
		pr_cont(" TT_SCSI_DMA");
	}
	/*
	 * The ST-DMA address registers aren't readable
	 * on all Medusas, so the test below may fail
	 */
	if (MACH_IS_MEDUSA ||
	    (hwreg_present(&st_dma.dma_vhi) &&
	     (st_dma.dma_vhi = 0x55) && (st_dma.dma_hi = 0xaa) &&
	     st_dma.dma_vhi == 0x55 && st_dma.dma_hi == 0xaa &&
	     (st_dma.dma_vhi = 0xaa) && (st_dma.dma_hi = 0x55) &&
	     st_dma.dma_vhi == 0xaa && st_dma.dma_hi == 0x55)) {
		ATARIHW_SET(EXTD_DMA);
		pr_cont(" EXTD_DMA");
	}
	if (hwreg_present(&tt_scsi.scsi_data)) {
		ATARIHW_SET(TT_SCSI);
		pr_cont(" TT_SCSI");
	}
	if (hwreg_present(&sound_ym.rd_data_reg_sel)) {
		ATARIHW_SET(YM_2149);
		pr_cont(" YM2149");
	}
	if (!MACH_IS_MEDUSA && hwreg_present(&tt_dmasnd.ctrl)) {
		ATARIHW_SET(PCM_8BIT);
		pr_cont(" PCM");
	}
	if (hwreg_present(&falcon_codec.unused5)) {
		ATARIHW_SET(CODEC);
		pr_cont(" CODEC");
	}
	if (hwreg_present(&dsp56k_host_interface.icr)) {
		ATARIHW_SET(DSP56K);
		pr_cont(" DSP56K");
	}
	if (hwreg_present(&tt_scc_dma.dma_ctrl) &&
#if 0
	    /* This test sucks! Who knows some better? */
	    (tt_scc_dma.dma_ctrl = 0x01, (tt_scc_dma.dma_ctrl & 1) == 1) &&
	    (tt_scc_dma.dma_ctrl = 0x00, (tt_scc_dma.dma_ctrl & 1) == 0)
#else
	    !MACH_IS_MEDUSA
#endif
	    ) {
		ATARIHW_SET(SCC_DMA);
		pr_cont(" SCC_DMA");
	}
	if (scc_test(&atari_scc.cha_a_ctrl)) {
		ATARIHW_SET(SCC);
		pr_cont(" SCC");
	}
	if (scc_test(&st_escc.cha_b_ctrl)) {
		ATARIHW_SET(ST_ESCC);
		pr_cont(" ST_ESCC");
	}
	if (hwreg_present(&tt_scu.sys_mask)) {
		ATARIHW_SET(SCU);
		/* Assume a VME bus if there's a SCU */
		ATARIHW_SET(VME);
		pr_cont(" VME SCU");
	}
	if (hwreg_present((void *)(0xffff9210))) {
		ATARIHW_SET(ANALOG_JOY);
		pr_cont(" ANALOG_JOY");
	}
	if (hwreg_present(blitter.halftone)) {
		ATARIHW_SET(BLITTER);
		pr_cont(" BLITTER");
	}
	if (hwreg_present((void *)0xfff00039)) {
		ATARIHW_SET(IDE);
		pr_cont(" IDE");
	}
#if 1 /* This maybe wrong */
	if (!MACH_IS_MEDUSA && hwreg_present(&tt_microwire.data) &&
	    hwreg_present(&tt_microwire.mask) &&
	    (tt_microwire.mask = 0x7ff,
	     udelay(1),
	     tt_microwire.data = MW_LM1992_PSG_HIGH | MW_LM1992_ADDR,
	     udelay(1),
	     tt_microwire.data != 0)) {
		ATARIHW_SET(MICROWIRE);
		while (tt_microwire.mask != 0x7ff)
			;
		pr_cont(" MICROWIRE");
	}
#endif
	if (hwreg_present(&tt_rtc.regsel)) {
		ATARIHW_SET(TT_CLK);
		pr_cont(" TT_CLK");
		mach_hwclk = atari_tt_hwclk;
	}
	if (hwreg_present(&mste_rtc.sec_ones)) {
		ATARIHW_SET(MSTE_CLK);
		pr_cont(" MSTE_CLK");
		mach_hwclk = atari_mste_hwclk;
	}
	if (!MACH_IS_MEDUSA && hwreg_present(&dma_wd.fdc_speed) &&
	    hwreg_write(&dma_wd.fdc_speed, 0)) {
		ATARIHW_SET(FDCSPEED);
		pr_cont(" FDC_SPEED");
	}
	if (!ATARIHW_PRESENT(ST_SCSI)) {
		ATARIHW_SET(ACSI);
		pr_cont(" ACSI");
	}
	pr_cont("\n");

	if (CPU_IS_040_OR_060)
		/* Now it seems to be safe to turn of the tt0 transparent
		 * translation (the one that must not be turned off in
		 * head.S...)
		 */
		asm volatile ("\n"
			"	moveq	#0,%%d0\n"
			"	.chip	68040\n"
			"	movec	%%d0,%%itt0\n"
			"	movec	%%d0,%%dtt0\n"
			"	.chip	68k"
			: /* no outputs */
			: /* no inputs */
			: "d0");

	/* allocator for memory that must reside in st-ram */
	atari_stram_init();

	/* Set up a mapping for the VMEbus address region:
	 *
	 * VME is either at phys. 0xfexxxxxx (TT) or 0xa00000..0xdfffff
	 * (MegaSTE) In both cases, the whole 16 MB chunk is mapped at
	 * 0xfe000000 virt., because this can be done with a single
	 * transparent translation. On the 68040, lots of often unused
	 * page tables would be needed otherwise. On a MegaSTE or similar,
	 * the highest byte is stripped off by hardware due to the 24 bit
	 * design of the bus.
	 */

	if (CPU_IS_020_OR_030) {
		unsigned long tt1_val;
		tt1_val = 0xfe008543;	/* Translate 0xfexxxxxx, enable, cache
					 * inhibit, read and write, FDC mask = 3,
					 * FDC val = 4 -> Supervisor only */
		asm volatile ("\n"
			"	.chip	68030\n"
			"	pmove	%0,%/tt1\n"
			"	.chip	68k"
			: : "m" (tt1_val));
	} else {
	        asm volatile ("\n"
			"	.chip	68040\n"
			"	movec	%0,%%itt1\n"
			"	movec	%0,%%dtt1\n"
			"	.chip	68k"
			:
			: "d" (0xfe00a040));	/* Translate 0xfexxxxxx, enable,
						 * supervisor only, non-cacheable/
						 * serialized, writable */

	}

	/* Fetch tos version at Physical 2 */
	/*
	 * We my not be able to access this address if the kernel is
	 * loaded to st ram, since the first page is unmapped.  On the
	 * Medusa this is always the case and there is nothing we can do
	 * about this, so we just assume the smaller offset.  For the TT
	 * we use the fact that in head.S we have set up a mapping
	 * 0xFFxxxxxx -> 0x00xxxxxx, so that the first 16MB is accessible
	 * in the last 16MB of the address space.
	 */
	tos_version = (MACH_IS_MEDUSA) ?
			0xfff : *(unsigned short *)0xff000002;
	atari_rtc_year_offset = (tos_version < 0x306) ? 70 : 68;
}
/* Context-After
 * #ifdef CONFIG_HEARTBEAT
 * static void atari_heartbeat(int on)
 * {
 * 	unsigned char tmp;
 * 	unsigned long flags;
 * 
 * 	if (atari_dont_touch_floppy_select)
 * 		return;
 * 
 * 	local_irq_save(flags);
 * 	sound_ym.rd_data_reg_sel = 14;	/* Select PSG Port A */
 * 	tmp = sound_ym.rd_data_reg_sel;
 * 	sound_ym.wd_data = on ? (tmp & ~0x02) : (tmp | 0x02);
 * 	local_irq_restore(flags);
 * }
 * #endif
 * 
 * /* ++roman:
 *  *
 */
