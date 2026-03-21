/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_002 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 16 */
/* NLOC: 70 */
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
 * 	if (scripting_ops) {
 * 		struct addr_location *addr_al_ptr = NULL;
 * 
 * 		if ((evsel->core.attr.sample_type & PERF_SAMPLE_ADDR) &&
 * 		    sample_addr_correlates_sym(&evsel->core.attr)) {
 * 			if (!addr_al.thread)
 * 				thread__resolve(al.thread, &addr_al, sample);
 * 			addr_al_ptr = &addr_al;
 * 		}
 * 		scripting_ops->process_event(event, sample, evsel, &al, addr_al_ptr);
 * 	} else {
 * 		process_event(scr, sample, evsel, &al, &addr_al, machine);
 * 	}
 * 
 * out_put:
 * 	addr_location__exit(&addr_al);
 * 	addr_location__exit(&al);
 * 	return ret;
 * }
 */
static int process_deferred_sample_event(const struct perf_tool *tool,
					 union perf_event *event,
					 struct perf_sample *sample,
					 struct evsel *evsel,
					 struct machine *machine)
{
	struct perf_script *scr = container_of(tool, struct perf_script, tool);
	struct perf_event_attr *attr = &evsel->core.attr;
	struct evsel_script *es = evsel->priv;
	unsigned int type = output_type(attr->type);
	struct addr_location al;
	FILE *fp = es->fp;
	int ret = 0;

	if (output[type].fields == 0)
		return 0;

	/* Set thread to NULL to indicate addr_al and al are not initialized */
	addr_location__init(&al);

	if (perf_time__ranges_skip_sample(scr->ptime_range, scr->range_num,
					  sample->time)) {
		goto out_put;
	}

	if (debug_mode) {
		if (sample->time < last_timestamp) {
			pr_err("Samples misordered, previous: %" PRIu64
				" this: %" PRIu64 "\n", last_timestamp,
				sample->time);
			nr_unordered++;
		}
		last_timestamp = sample->time;
		goto out_put;
	}

	if (filter_cpu(sample))
		goto out_put;

	if (machine__resolve(machine, &al, sample) < 0) {
		pr_err("problem processing %d event, skipping it.\n",
		       event->header.type);
		ret = -1;
		goto out_put;
	}

	if (al.filtered)
		goto out_put;

	if (!show_event(sample, evsel, al.thread, &al, NULL))
		goto out_put;

	if (evswitch__discard(&scr->evswitch, evsel))
		goto out_put;

	perf_sample__fprintf_start(scr, sample, al.thread, evsel,
				   PERF_RECORD_CALLCHAIN_DEFERRED, fp);
	fprintf(fp, "DEFERRED CALLCHAIN [cookie: %llx]",
		(unsigned long long)event->callchain_deferred.cookie);

	if (PRINT_FIELD(IP)) {
		struct callchain_cursor *cursor = NULL;

		if (symbol_conf.use_callchain && sample->callchain) {
			cursor = get_tls_callchain_cursor();
			if (thread__resolve_callchain(al.thread, cursor, evsel,
						      sample, NULL, NULL,
						      scripting_max_stack)) {
				pr_info("cannot resolve deferred callchains\n");
				cursor = NULL;
			}
		}

		fputc(cursor ? '\n' : ' ', fp);
		sample__fprintf_sym(sample, &al, 0, output[type].print_ip_opts,
				    cursor, symbol_conf.bt_stop_list, fp);
	}

	fprintf(fp, "\n");

	if (verbose > 0)
		fflush(fp);

out_put:
	addr_location__exit(&al);
	return ret;
}
/* Context-After
 * // Used when scr->per_event_dump is not set
 * static struct evsel_script es_stdout;
 * 
 * static int process_attr(const struct perf_tool *tool, union perf_event *event,
 * 			struct evlist **pevlist)
 * {
 * 	struct perf_script *scr = container_of(tool, struct perf_script, tool);
 * 	struct evlist *evlist;
 * 	struct evsel *evsel, *pos;
 * 	uint16_t e_machine;
 * 	u64 sample_type;
 * 	int err;
 * 
 * 	err = perf_event__process_attr(tool, event, pevlist);
 * 	if (err)
 * 		return err;
 * 
 * 	evlist = *pevlist;
 * 	evsel = evlist__last(*pevlist);
 */
