/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_030 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 42 */
/* NLOC: 108 */
/* Subsystem: tools */
/* Includes
 * #include "builtin.h"
 * #include "util/counts.h"
 * #include "util/debug.h"
 * #include "util/dso.h"
 * #include <subcmd/exec-cmd.h>
 * #include "util/header.h"
 * #include <subcmd/parse-options.h>
 * #include "util/perf_regs.h"
 * #include "util/session.h"
 * #include "util/tool.h"
 * #include "util/map.h"
 * #include "util/srcline.h"
 * #include "util/symbol.h"
 * #include "util/thread.h"
 * #include "util/trace-event.h"
 * #include "util/env.h"
 * #include "util/evlist.h"
 * #include "util/evsel.h"
 * #include "util/evsel_fprintf.h"
 * #include "util/evswitch.h"
 */
/* Context-Before
 * 	if (!sample->intr_regs)
 * 		return 0;
 * 
 * 	return perf_sample__fprintf_regs(perf_sample__intr_regs(sample),
 * 					 attr->sample_regs_intr, e_machine, e_flags, fp);
 * }
 * 
 * static int perf_sample__fprintf_uregs(struct perf_sample *sample,
 * 				      struct perf_event_attr *attr,
 * 				      uint16_t e_machine,
 * 				      uint32_t e_flags,
 * 				      FILE *fp)
 * {
 * 	if (!sample->user_regs)
 * 		return 0;
 * 
 * 	return perf_sample__fprintf_regs(perf_sample__user_regs(sample),
 * 					 attr->sample_regs_user, e_machine, e_flags, fp);
 * }
 */
static int perf_sample__fprintf_start(struct perf_script *script,
				      struct perf_sample *sample,
				      struct thread *thread,
				      struct evsel *evsel,
				      u32 type, FILE *fp)
{
	unsigned long secs;
	unsigned long long nsecs;
	int printed = 0;
	char tstr[128];

	/*
	 * Print the branch counter's abbreviation list,
	 * if the branch counter is available.
	 */
	if (PRINT_FIELD(BRCNTR) && !verbose) {
		char *buf;

		if (!annotation_br_cntr_abbr_list(&buf, evsel, true)) {
			printed += fprintf(stdout, "%s", buf);
			free(buf);
		}
	}

	if (PRINT_FIELD(MACHINE_PID) && sample->machine_pid)
		printed += fprintf(fp, "VM:%5d ", sample->machine_pid);

	/* Print VCPU only for guest events i.e. with machine_pid */
	if (PRINT_FIELD(VCPU) && sample->machine_pid)
		printed += fprintf(fp, "VCPU:%03d ", sample->vcpu);

	if (PRINT_FIELD(COMM)) {
		const char *comm = thread ? thread__comm_str(thread) : ":-1";

		if (latency_format)
			printed += fprintf(fp, "%8.8s ", comm);
		else if (PRINT_FIELD(IP) && evsel__has_callchain(evsel) && symbol_conf.use_callchain)
			printed += fprintf(fp, "%s ", comm);
		else
			printed += fprintf(fp, "%16s ", comm);
	}

	if (PRINT_FIELD(PID) && PRINT_FIELD(TID))
		printed += fprintf(fp, "%7d/%-7d ", sample->pid, sample->tid);
	else if (PRINT_FIELD(PID))
		printed += fprintf(fp, "%7d ", sample->pid);
	else if (PRINT_FIELD(TID))
		printed += fprintf(fp, "%7d ", sample->tid);

	if (PRINT_FIELD(CPU)) {
		if (latency_format)
			printed += fprintf(fp, "%3d ", sample->cpu);
		else
			printed += fprintf(fp, "[%03d] ", sample->cpu);
	}

	if (PRINT_FIELD(MISC)) {
		int ret = 0;

		#define has(m) \
			(sample->misc & PERF_RECORD_MISC_##m) == PERF_RECORD_MISC_##m

		if (has(KERNEL))
			ret += fprintf(fp, "K");
		if (has(USER))
			ret += fprintf(fp, "U");
		if (has(HYPERVISOR))
			ret += fprintf(fp, "H");
		if (has(GUEST_KERNEL))
			ret += fprintf(fp, "G");
		if (has(GUEST_USER))
			ret += fprintf(fp, "g");

		switch (type) {
		case PERF_RECORD_MMAP:
		case PERF_RECORD_MMAP2:
			if (has(MMAP_DATA))
				ret += fprintf(fp, "M");
			break;
		case PERF_RECORD_COMM:
			if (has(COMM_EXEC))
				ret += fprintf(fp, "E");
			break;
		case PERF_RECORD_SWITCH:
		case PERF_RECORD_SWITCH_CPU_WIDE:
			if (has(SWITCH_OUT)) {
				ret += fprintf(fp, "S");
				if (sample->misc & PERF_RECORD_MISC_SWITCH_OUT_PREEMPT)
					ret += fprintf(fp, "p");
			}
		default:
			break;
		}

		#undef has

		ret += fprintf(fp, "%*s", 6 - ret, " ");
		printed += ret;
	}

	if (PRINT_FIELD(TOD)) {
		tod_scnprintf(script, tstr, sizeof(tstr), sample->time);
		printed += fprintf(fp, "%s ", tstr);
	}

	if (PRINT_FIELD(TIME)) {
		u64 t = sample->time;
		if (reltime) {
			if (!initial_time)
				initial_time = sample->time;
			t = sample->time - initial_time;
		} else if (deltatime) {
			if (previous_time)
				t = sample->time - previous_time;
			else {
				t = 0;
			}
			previous_time = sample->time;
		}
		nsecs = t;
		secs = nsecs / NSEC_PER_SEC;
		nsecs -= secs * NSEC_PER_SEC;

		if (symbol_conf.nanosecs)
			printed += fprintf(fp, "%5lu.%09llu: ", secs, nsecs);
		else {
			char sample_time[32];
			timestamp__scnprintf_usec(t, sample_time, sizeof(sample_time));
			printed += fprintf(fp, "%12s: ", sample_time);
		}
	}

	return printed;
}
/* Context-After
 * static inline size_t
 * bstack_event_str(struct branch_entry *br, char *buf, size_t sz)
 * {
 * 	if (!(br->flags.mispred || br->flags.predicted || br->flags.not_taken))
 * 		return snprintf(buf, sz, "-");
 * 
 * 	return snprintf(buf, sz, "%s%s",
 * 			br->flags.predicted ? "P" : "M",
 * 			br->flags.not_taken ? "N" : "");
 * }
 * 
 * static int print_bstack_flags(FILE *fp, struct branch_entry *br)
 * {
 * 	char events[16] = { 0 };
 * 	size_t pos;
 * 
 * 	pos = bstack_event_str(br, events, sizeof(events));
 * 	return fprintf(fp, "/%s/%c/%c/%d/%s/%s ",
 * 		       pos < 0 ? "-" : events,
 */
