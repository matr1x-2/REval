/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_006 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 12 */
/* NLOC: 60 */
/* Subsystem: arch */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/sched.h>
 * #include <linux/sched/clock.h>
 * #include <linux/init.h>
 * #include <linux/export.h>
 * #include <linux/timer.h>
 * #include <linux/acpi_pmtmr.h>
 * #include <linux/cpufreq.h>
 * #include <linux/delay.h>
 * #include <linux/clocksource.h>
 * #include <linux/kvm_types.h>
 * #include <linux/percpu.h>
 * #include <linux/timex.h>
 * #include <linux/static_key.h>
 * #include <linux/static_call.h>
 * #include <asm/cpuid/api.h>
 * #include <asm/hpet.h>
 * #include <asm/timer.h>
 * #include <asm/vgtod.h>
 * #include <asm/time.h>
 */
/* Context-Before
 * 	return 0;
 * }
 * 
 * static void tsc_refine_calibration_work(struct work_struct *work);
 * static DECLARE_DELAYED_WORK(tsc_irqwork, tsc_refine_calibration_work);
 * /**
 *  * tsc_refine_calibration_work - Further refine tsc freq calibration
 *  * @work: ignored.
 *  *
 *  * This functions uses delayed work over a period of a
 *  * second to further refine the TSC freq value. Since this is
 *  * timer based, instead of loop based, we don't block the boot
 *  * process while this longer calibration is done.
 *  *
 *  * If there are any calibration anomalies (too many SMIs, etc),
 *  * or the refined calibration is off by 1% of the fast early
 *  * calibration, we throw out the new calibration and use the
 *  * early calibration.
 *  */
 */
static void tsc_refine_calibration_work(struct work_struct *work)
{
	static u64 tsc_start = ULLONG_MAX, ref_start;
	static int hpet;
	u64 tsc_stop, ref_stop, delta;
	unsigned long freq;
	int cpu;

	/* Don't bother refining TSC on unstable systems */
	if (tsc_unstable)
		goto unreg;

	/*
	 * Since the work is started early in boot, we may be
	 * delayed the first time we expire. So set the workqueue
	 * again once we know timers are working.
	 */
	if (tsc_start == ULLONG_MAX) {
restart:
		/*
		 * Only set hpet once, to avoid mixing hardware
		 * if the hpet becomes enabled later.
		 */
		hpet = is_hpet_enabled();
		tsc_start = tsc_read_refs(&ref_start, hpet);
		schedule_delayed_work(&tsc_irqwork, HZ);
		return;
	}

	tsc_stop = tsc_read_refs(&ref_stop, hpet);

	/* hpet or pmtimer available ? */
	if (ref_start == ref_stop)
		goto out;

	/* Check, whether the sampling was disturbed */
	if (tsc_stop == ULLONG_MAX)
		goto restart;

	delta = tsc_stop - tsc_start;
	delta *= 1000000LL;
	if (hpet)
		freq = calc_hpet_ref(delta, ref_start, ref_stop);
	else
		freq = calc_pmtimer_ref(delta, ref_start, ref_stop);

	/* Will hit this only if tsc_force_recalibrate has been set */
	if (boot_cpu_has(X86_FEATURE_TSC_KNOWN_FREQ)) {

		/* Warn if the deviation exceeds 500 ppm */
		if (abs(tsc_khz - freq) > (tsc_khz >> 11)) {
			pr_warn("Warning: TSC freq calibrated by CPUID/MSR differs from what is calibrated by HW timer, please check with vendor!!\n");
			pr_info("Previous calibrated TSC freq:\t %lu.%03lu MHz\n",
				(unsigned long)tsc_khz / 1000,
				(unsigned long)tsc_khz % 1000);
		}

		pr_info("TSC freq recalibrated by [%s]:\t %lu.%03lu MHz\n",
			hpet ? "HPET" : "PM_TIMER",
			(unsigned long)freq / 1000,
			(unsigned long)freq % 1000);

		return;
	}

	/* Make sure we're within 1% */
	if (abs(tsc_khz - freq) > tsc_khz/100)
		goto out;

	tsc_khz = freq;
	pr_info("Refined TSC clocksource calibration: %lu.%03lu MHz\n",
		(unsigned long)tsc_khz / 1000,
		(unsigned long)tsc_khz % 1000);

	/* Inform the TSC deadline clockevent devices about the recalibration */
	lapic_update_tsc_freq();

	/* Update the sched_clock() rate to match the clocksource one */
	for_each_possible_cpu(cpu)
		set_cyc2ns_scale(tsc_khz, cpu, tsc_stop);

out:
	if (tsc_unstable)
		goto unreg;

	if (boot_cpu_has(X86_FEATURE_ART)) {
		have_art = true;
		clocksource_tsc.base = &art_base_clk;
	}
	clocksource_register_khz(&clocksource_tsc, tsc_khz);
unreg:
	clocksource_unregister(&clocksource_tsc_early);
}
/* Context-After
 * static int __init init_tsc_clocksource(void)
 * {
 * 	if (!boot_cpu_has(X86_FEATURE_TSC) || !tsc_khz)
 * 		return 0;
 * 
 * 	if (tsc_unstable) {
 * 		clocksource_unregister(&clocksource_tsc_early);
 * 		return 0;
 * 	}
 * 
 * 	if (boot_cpu_has(X86_FEATURE_NONSTOP_TSC_S3))
 * 		clocksource_tsc.flags |= CLOCK_SOURCE_SUSPEND_NONSTOP;
 * 
 * 	/*
 * 	 * When TSC frequency is known (retrieved via MSR or CPUID), we skip
 * 	 * the refined calibration and directly register it as a clocksource.
 * 	 */
 * 	if (boot_cpu_has(X86_FEATURE_TSC_KNOWN_FREQ)) {
 */
