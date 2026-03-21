/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_039 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 82 */
/* NLOC: 191 */
/* Subsystem: arch */
/* Includes
 * #include <linux/stddef.h>
 * #include <linux/types.h>
 * #include <linux/init.h>
 * #include <linux/slab.h>
 * #include <linux/export.h>
 * #include <linux/nmi.h>
 * #include <linux/kvm_host.h>
 * #include <asm/cpufeature.h>
 * #include <asm/debugreg.h>
 * #include <asm/hardirq.h>
 * #include <asm/intel-family.h>
 * #include <asm/intel_pt.h>
 * #include <asm/apic.h>
 * #include <asm/cpu_device_id.h>
 * #include <asm/msr.h>
 * #include "../perf_event.h"
 */
/* Context-Before
 * 		return true;
 * 
 * 	return false;
 * }
 * 
 * static inline void intel_pmu_set_acr_cntr_constr(struct perf_event *event,
 * 						 u64 *cause_mask, int *num)
 * {
 * 	event->hw.dyn_constraint &= hybrid(event->pmu, acr_cntr_mask64);
 * 	*cause_mask |= event->attr.config2;
 * 	*num += 1;
 * }
 * 
 * static inline void intel_pmu_set_acr_caused_constr(struct perf_event *event,
 * 						   int idx, u64 cause_mask)
 * {
 * 	if (test_bit(idx, (unsigned long *)&cause_mask))
 * 		event->hw.dyn_constraint &= hybrid(event->pmu, acr_cause_mask64);
 * }
 */
static int intel_pmu_hw_config(struct perf_event *event)
{
	int ret = x86_pmu_hw_config(event);

	if (ret)
		return ret;

	ret = intel_pmu_bts_config(event);
	if (ret)
		return ret;

	if (event->attr.freq && event->attr.sample_freq) {
		event->hw.sample_period = intel_pmu_freq_start_period(event);
		event->hw.last_period = event->hw.sample_period;
		local64_set(&event->hw.period_left, event->hw.sample_period);
	}

	if (event->attr.precise_ip) {
		struct arch_pebs_cap pebs_cap = hybrid(event->pmu, arch_pebs_cap);

		if ((event->attr.config & INTEL_ARCH_EVENT_MASK) == INTEL_FIXED_VLBR_EVENT)
			return -EINVAL;

		if (!(event->attr.freq || (event->attr.wakeup_events && !event->attr.watermark))) {
			event->hw.flags |= PERF_X86_EVENT_AUTO_RELOAD;
			if (!(event->attr.sample_type & ~intel_pmu_large_pebs_flags(event)) &&
			    !has_aux_action(event)) {
				event->hw.flags |= PERF_X86_EVENT_LARGE_PEBS;
				event->attach_state |= PERF_ATTACH_SCHED_CB;
			}
		}
		if (x86_pmu.pebs_aliases)
			x86_pmu.pebs_aliases(event);

		if (x86_pmu.arch_pebs) {
			u64 cntr_mask = hybrid(event->pmu, intel_ctrl) &
						~GLOBAL_CTRL_EN_PERF_METRICS;
			u64 pebs_mask = event->attr.precise_ip >= 3 ?
						pebs_cap.pdists : pebs_cap.counters;
			if (cntr_mask != pebs_mask)
				event->hw.dyn_constraint &= pebs_mask;
		}
	}

	if (needs_branch_stack(event)) {
		/* Avoid branch stack setup for counting events in SAMPLE READ */
		if (is_sampling_event(event) ||
		    !(event->attr.sample_type & PERF_SAMPLE_READ))
			event->hw.flags |= PERF_X86_EVENT_NEEDS_BRANCH_STACK;
	}

	if (branch_sample_counters(event)) {
		struct perf_event *leader, *sibling;
		int num = 0;

		if (!(x86_pmu.flags & PMU_FL_BR_CNTR) ||
		    (event->attr.config & ~INTEL_ARCH_EVENT_MASK))
			return -EINVAL;

		/*
		 * The branch counter logging is not supported in the call stack
		 * mode yet, since we cannot simply flush the LBR during e.g.,
		 * multiplexing. Also, there is no obvious usage with the call
		 * stack mode. Simply forbids it for now.
		 *
		 * If any events in the group enable the branch counter logging
		 * feature, the group is treated as a branch counter logging
		 * group, which requires the extra space to store the counters.
		 */
		leader = event->group_leader;
		if (branch_sample_call_stack(leader))
			return -EINVAL;
		if (branch_sample_counters(leader)) {
			num++;
			leader->hw.dyn_constraint &= x86_pmu.lbr_counters;
		}
		leader->hw.flags |= PERF_X86_EVENT_BRANCH_COUNTERS;

		for_each_sibling_event(sibling, leader) {
			if (branch_sample_call_stack(sibling))
				return -EINVAL;
			if (branch_sample_counters(sibling)) {
				num++;
				sibling->hw.dyn_constraint &= x86_pmu.lbr_counters;
			}
		}

		if (num > fls(x86_pmu.lbr_counters))
			return -EINVAL;
		/*
		 * Only applying the PERF_SAMPLE_BRANCH_COUNTERS doesn't
		 * require any branch stack setup.
		 * Clear the bit to avoid unnecessary branch stack setup.
		 */
		if (0 == (event->attr.branch_sample_type &
			  ~(PERF_SAMPLE_BRANCH_PLM_ALL |
			    PERF_SAMPLE_BRANCH_COUNTERS)))
			event->hw.flags  &= ~PERF_X86_EVENT_NEEDS_BRANCH_STACK;

		/*
		 * Force the leader to be a LBR event. So LBRs can be reset
		 * with the leader event. See intel_pmu_lbr_del() for details.
		 */
		if (!intel_pmu_needs_branch_stack(leader))
			return -EINVAL;
	}

	if (intel_pmu_needs_branch_stack(event)) {
		ret = intel_pmu_setup_lbr_filter(event);
		if (ret)
			return ret;
		event->attach_state |= PERF_ATTACH_SCHED_CB;

		/*
		 * BTS is set up earlier in this path, so don't account twice
		 */
		if (!unlikely(intel_pmu_has_bts(event))) {
			/* disallow lbr if conflicting events are present */
			if (x86_add_exclusive(x86_lbr_exclusive_lbr))
				return -EBUSY;

			event->destroy = hw_perf_lbr_event_destroy;
		}
	}

	if (event->attr.aux_output) {
		if (!event->attr.precise_ip)
			return -EINVAL;

		event->hw.flags |= PERF_X86_EVENT_PEBS_VIA_PT;
	}

	if ((event->attr.sample_type & PERF_SAMPLE_READ) &&
	    intel_pmu_has_pebs_counter_group(event->pmu) &&
	    is_sampling_event(event) &&
	    event->attr.precise_ip)
		event->group_leader->hw.flags |= PERF_X86_EVENT_PEBS_CNTR;

	if (intel_pmu_has_acr(event->pmu) && intel_pmu_is_acr_group(event)) {
		struct perf_event *sibling, *leader = event->group_leader;
		struct pmu *pmu = event->pmu;
		bool has_sw_event = false;
		int num = 0, idx = 0;
		u64 cause_mask = 0;

		/* Not support perf metrics */
		if (is_metric_event(event))
			return -EINVAL;

		/* Not support freq mode */
		if (event->attr.freq)
			return -EINVAL;

		/* PDist is not supported */
		if (event->attr.config2 && event->attr.precise_ip > 2)
			return -EINVAL;

		/* The reload value cannot exceeds the max period */
		if (event->attr.sample_period > x86_pmu.max_period)
			return -EINVAL;
		/*
		 * The counter-constraints of each event cannot be finalized
		 * unless the whole group is scanned. However, it's hard
		 * to know whether the event is the last one of the group.
		 * Recalculate the counter-constraints for each event when
		 * adding a new event.
		 *
		 * The group is traversed twice, which may be optimized later.
		 * In the first round,
		 * - Find all events which do reload when other events
		 *   overflow and set the corresponding counter-constraints
		 * - Add all events, which can cause other events reload,
		 *   in the cause_mask
		 * - Error out if the number of events exceeds the HW limit
		 * - The ACR events must be contiguous.
		 *   Error out if there are non-X86 events between ACR events.
		 *   This is not a HW limit, but a SW limit.
		 *   With the assumption, the intel_pmu_acr_late_setup() can
		 *   easily convert the event idx to counter idx without
		 *   traversing the whole event list.
		 */
		if (!is_x86_event(leader))
			return -EINVAL;

		if (leader->attr.config2)
			intel_pmu_set_acr_cntr_constr(leader, &cause_mask, &num);

		if (leader->nr_siblings) {
			for_each_sibling_event(sibling, leader) {
				if (!is_x86_event(sibling)) {
					has_sw_event = true;
					continue;
				}
				if (!sibling->attr.config2)
					continue;
				if (has_sw_event)
					return -EINVAL;
				intel_pmu_set_acr_cntr_constr(sibling, &cause_mask, &num);
			}
		}
		if (leader != event && event->attr.config2) {
			if (has_sw_event)
				return -EINVAL;
			intel_pmu_set_acr_cntr_constr(event, &cause_mask, &num);
		}

		if (hweight64(cause_mask) > hweight64(hybrid(pmu, acr_cause_mask64)) ||
		    num > hweight64(hybrid(event->pmu, acr_cntr_mask64)))
			return -EINVAL;
		/*
		 * In the second round, apply the counter-constraints for
		 * the events which can cause other events reload.
		 */
		intel_pmu_set_acr_caused_constr(leader, idx++, cause_mask);

		if (leader->nr_siblings) {
			for_each_sibling_event(sibling, leader)
				intel_pmu_set_acr_caused_constr(sibling, idx++, cause_mask);
		}

		if (leader != event)
			intel_pmu_set_acr_caused_constr(event, idx, cause_mask);

		leader->hw.flags |= PERF_X86_EVENT_ACR;
	}

	if ((event->attr.type == PERF_TYPE_HARDWARE) ||
	    (event->attr.type == PERF_TYPE_HW_CACHE))
		return 0;

	/*
	 * Config Topdown slots and metric events
	 *
	 * The slots event on Fixed Counter 3 can support sampling,
	 * which will be handled normally in x86_perf_event_update().
	 *
	 * Metric events don't support sampling and require being paired
	 * with a slots event as group leader. When the slots event
	 * is used in a metrics group, it too cannot support sampling.
	 */
	if (intel_pmu_has_cap(event, PERF_CAP_METRICS_IDX) && is_topdown_event(event)) {
		/* The metrics_clear can only be set for the slots event */
		if (event->attr.config1 &&
		    (!is_slots_event(event) || (event->attr.config1 & ~INTEL_TD_CFG_METRIC_CLEAR)))
			return -EINVAL;

		if (event->attr.config2)
			return -EINVAL;

		/*
		 * The TopDown metrics events and slots event don't
		 * support any filters.
		 */
		if (event->attr.config & X86_ALL_EVENT_FLAGS)
			return -EINVAL;

		if (is_available_metric_event(event)) {
			struct perf_event *leader = event->group_leader;

			/* The metric events don't support sampling. */
			if (is_sampling_event(event))
				return -EINVAL;

			/* The metric events require a slots group leader. */
			if (!is_slots_event(leader))
				return -EINVAL;

			/*
			 * The leader/SLOTS must not be a sampling event for
			 * metric use; hardware requires it starts at 0 when used
			 * in conjunction with MSR_PERF_METRICS.
			 */
			if (is_sampling_event(leader))
				return -EINVAL;

			event->event_caps |= PERF_EV_CAP_SIBLING;
			/*
			 * Only once we have a METRICs sibling do we
			 * need TopDown magic.
			 */
			leader->hw.flags |= PERF_X86_EVENT_TOPDOWN;
			event->hw.flags  |= PERF_X86_EVENT_TOPDOWN;
		}
	}

	/*
	 * The load latency event X86_CONFIG(.event=0xcd, .umask=0x01) on SPR
	 * doesn't function quite right. As a work-around it needs to always be
	 * co-scheduled with a auxiliary event X86_CONFIG(.event=0x03, .umask=0x82).
	 * The actual count of this second event is irrelevant it just needs
	 * to be active to make the first event function correctly.
	 *
	 * In a group, the auxiliary event must be in front of the load latency
	 * event. The rule is to simplify the implementation of the check.
	 * That's because perf cannot have a complete group at the moment.
	 */
	if (require_mem_loads_aux_event(event) &&
	    (event->attr.sample_type & PERF_SAMPLE_DATA_SRC) &&
	    is_mem_loads_event(event)) {
		struct perf_event *leader = event->group_leader;
		struct perf_event *sibling = NULL;

		/*
		 * When this memload event is also the first event (no group
		 * exists yet), then there is no aux event before it.
		 */
		if (leader == event)
			return -ENODATA;

		if (!is_mem_loads_aux_event(leader)) {
			for_each_sibling_event(sibling, leader) {
				if (is_mem_loads_aux_event(sibling))
					break;
			}
			if (list_entry_is_head(sibling, &leader->sibling_list, sibling_list))
				return -ENODATA;
		}
	}

	if (!(event->attr.config & ARCH_PERFMON_EVENTSEL_ANY))
		return 0;

	if (x86_pmu.version < 3)
		return -EINVAL;

	ret = perf_allow_cpu();
	if (ret)
		return ret;

	event->hw.config |= ARCH_PERFMON_EVENTSEL_ANY;

	return 0;
}
/* Context-After
 * /*
 *  * Currently, the only caller of this function is the atomic_switch_perf_msrs().
 *  * The host perf context helps to prepare the values of the real hardware for
 *  * a set of msrs that need to be switched atomically in a vmx transaction.
 *  *
 *  * For example, the pseudocode needed to add a new msr should look like:
 *  *
 *  * arr[(*nr)++] = (struct perf_guest_switch_msr){
 *  *	.msr = the hardware msr address,
 *  *	.host = the value the hardware has when it doesn't run a guest,
 *  *	.guest = the value the hardware has when it runs a guest,
 *  * };
 *  *
 *  * These values have nothing to do with the emulated values the guest sees
 *  * when it uses {RD,WR}MSR, which should be handled by the KVM context,
 *  * specifically in the intel_pmu_{get,set}_msr().
 *  */
 * static struct perf_guest_switch_msr *intel_guest_get_msrs(int *nr, void *data)
 * {
 */
