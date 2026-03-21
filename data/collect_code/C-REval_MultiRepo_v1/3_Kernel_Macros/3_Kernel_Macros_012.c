/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_012 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 16 */
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
 * 			     union perf_event *event)
 * {
 * 	struct perf_script *script = container_of(tool, struct perf_script, tool);
 * 
 * 	if (dump_trace)
 * 		perf_event__fprintf_thread_map(event, stdout);
 * 
 * 	if (script->threads) {
 * 		pr_warning("Extra thread map event, ignoring.\n");
 * 		return 0;
 * 	}
 * 
 * 	script->threads = thread_map__new_event(&event->thread_map);
 * 	if (!script->threads)
 * 		return -ENOMEM;
 * 
 * 	return set_maps(script);
 * }
 * 
 * static
 */
int process_cpu_map_event(const struct perf_tool *tool,
			  struct perf_session *session __maybe_unused,
			  union perf_event *event)
{
	struct perf_script *script = container_of(tool, struct perf_script, tool);

	if (dump_trace)
		perf_event__fprintf_cpu_map(event, stdout);

	if (script->cpus) {
		pr_warning("Extra cpu map event, ignoring.\n");
		return 0;
	}

	script->cpus = cpu_map__new_data(&event->cpu_map.data);
	if (!script->cpus)
		return -ENOMEM;

	return set_maps(script);
}
/* Context-After
 * static int process_feature_event(const struct perf_tool *tool __maybe_unused,
 * 				 struct perf_session *session,
 * 				 union perf_event *event)
 * {
 * 	if (event->feat.feat_id < HEADER_LAST_FEATURE)
 * 		return perf_event__process_feature(session, event);
 * 	return 0;
 * }
 * 
 * static int perf_script__process_auxtrace_info(const struct perf_tool *tool,
 * 					      struct perf_session *session,
 * 					      union perf_event *event)
 * {
 * 	int ret = perf_event__process_auxtrace_info(tool, session, event);
 * 
 * 	if (ret == 0) {
 * 		struct perf_script *script = container_of(tool, struct perf_script, tool);
 * 
 * 		ret = perf_script__setup_per_event_dump(script);
 */
