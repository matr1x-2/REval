/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_026 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 28 */
/* NLOC: 110 */
/* Subsystem: kernel */
/* Includes
 * #include <linux/cleanup.h>
 * #include <linux/ftrace.h>
 * #include <linux/module.h>
 * #include <linux/mutex.h>
 * #include "trace_dynevent.h"
 * #include "trace_probe.h"
 * #include "trace_probe_kernel.h"
 * #include "trace_probe_tmpl.h"
 */
/* Context-Before
 * 		else
 * 			ret = snprintf(p, len, "%s", argv[i]);
 * 		p += ret;
 * 		len -= ret;
 * 	}
 * 
 * 	/*
 * 	 * Ensure the filter string can be parsed correctly. Note, this
 * 	 * filter string is for the original event, not for the eprobe.
 * 	 */
 * 	ret = create_event_filter(top_trace_array(), ep->event, ep->filter_str,
 * 				  true, &dummy);
 * 	free_event_filter(dummy);
 * 	if (ret) {
 * 		kfree(ep->filter_str);
 * 		ep->filter_str = NULL;
 * 	}
 * 	return ret;
 * }
 */
static int __trace_eprobe_create(int argc, const char *argv[])
{
	/*
	 * Argument syntax:
	 *      e[:[GRP/][ENAME]] SYSTEM.EVENT [FETCHARGS] [if FILTER]
	 * Fetch args (no space):
	 *  <name>=$<field>[:TYPE]
	 */
	struct traceprobe_parse_context *ctx __free(traceprobe_parse_context) = NULL;
	struct trace_eprobe *ep __free(trace_event_probe_cleanup) = NULL;
	const char *trlog __free(trace_probe_log_clear) = NULL;
	const char *event = NULL, *group = EPROBE_EVENT_SYSTEM;
	const char *sys_event = NULL, *sys_name = NULL;
	struct trace_event_call *event_call;
	char *buf1 __free(kfree) = NULL;
	char *buf2 __free(kfree) = NULL;
	char *gbuf __free(kfree) = NULL;
	int ret = 0, filter_idx = 0;
	int i, filter_cnt;

	if (argc < 2 || argv[0][0] != 'e')
		return -ECANCELED;

	trlog = trace_probe_log_init("event_probe", argc, argv);

	event = strchr(&argv[0][1], ':');
	if (event) {
		gbuf = kmalloc(MAX_EVENT_NAME_LEN, GFP_KERNEL);
		if (!gbuf)
			return -ENOMEM;
		event++;
		ret = traceprobe_parse_event_name(&event, &group, gbuf,
						  event - argv[0]);
		if (ret)
			return -EINVAL;
	}

	trace_probe_log_set_index(1);
	sys_event = argv[1];

	buf2 = kmalloc(MAX_EVENT_NAME_LEN, GFP_KERNEL);
	if (!buf2)
		return -ENOMEM;

	ret = traceprobe_parse_event_name(&sys_event, &sys_name, buf2, 0);
	if (ret || !sys_event || !sys_name) {
		trace_probe_log_err(0, NO_EVENT_INFO);
		return -EINVAL;
	}

	if (!event) {
		buf1 = kstrdup(sys_event, GFP_KERNEL);
		if (!buf1)
			return -ENOMEM;
		event = buf1;
	}

	for (i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "if")) {
			filter_idx = i + 1;
			filter_cnt = argc - filter_idx;
			argc = i;
			break;
		}
	}

	if (argc - 2 > MAX_TRACE_ARGS) {
		trace_probe_log_set_index(2);
		trace_probe_log_err(0, TOO_MANY_ARGS);
		return -E2BIG;
	}

	scoped_guard(mutex, &event_mutex) {
		event_call = find_and_get_event(sys_name, sys_event);
		ep = alloc_event_probe(group, event, event_call, argc - 2);
	}

	if (IS_ERR(ep)) {
		ret = PTR_ERR(ep);
		if (ret == -ENODEV)
			trace_probe_log_err(0, BAD_ATTACH_EVENT);
		/* This must return -ENOMEM or missing event, else there is a bug */
		WARN_ON_ONCE(ret != -ENOMEM && ret != -ENODEV);
		return ret;
	}

	if (filter_idx) {
		trace_probe_log_set_index(filter_idx);
		ret = trace_eprobe_parse_filter(ep, filter_cnt, argv + filter_idx);
		if (ret)
			return -EINVAL;
	} else
		ep->filter_str = NULL;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->event = ep->event;
	ctx->flags = TPARG_FL_KERNEL | TPARG_FL_TEVENT;

	argc -= 2; argv += 2;
	/* parse arguments */
	for (i = 0; i < argc; i++) {
		trace_probe_log_set_index(i + 2);

		ret = traceprobe_parse_probe_arg(&ep->tp, i, argv[i], ctx);
		/* Handle symbols "@" */
		if (!ret)
			ret = traceprobe_update_arg(&ep->tp.args[i]);
		if (ret)
			return ret;
	}
	ret = traceprobe_set_print_fmt(&ep->tp, PROBE_PRINT_EVENT);
	if (ret < 0)
		return ret;

	init_trace_eprobe_call(ep);
	scoped_guard(mutex, &event_mutex) {
		ret = trace_probe_register_event_call(&ep->tp);
		if (ret) {
			if (ret == -EEXIST) {
				trace_probe_log_set_index(0);
				trace_probe_log_err(0, EVENT_EXIST);
			}
			return ret;
		}
		ret = dyn_event_add(&ep->devent, &ep->tp.event->call);
		if (ret < 0) {
			trace_probe_unregister_event_call(&ep->tp);
			return ret;
		}
		/* To avoid freeing registered eprobe event, clear ep. */
		ep = NULL;
	}
	return ret;
}
/* Context-After
 * /*
 *  * Register dynevent at core_initcall. This allows kernel to setup eprobe
 *  * events in postcore_initcall without tracefs.
 *  */
 * static __init int trace_events_eprobe_init_early(void)
 * {
 * 	int err = 0;
 * 
 * 	err = dyn_event_register(&eprobe_dyn_event_ops);
 * 	if (err)
 * 		pr_warn("Could not register eprobe_dyn_event_ops\n");
 * 
 * 	return err;
 * }
 * core_initcall(trace_events_eprobe_init_early);
 */
