/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_030 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 36 */
/* Subsystem: samples */
/* Includes
 * #include <stdio.h>
 * #include <unistd.h>
 * #include <stdlib.h>
 * #include <stdbool.h>
 * #include <string.h>
 * #include <linux/perf_event.h>
 * #include <linux/bpf.h>
 * #include <signal.h>
 * #include <errno.h>
 * #include <sys/resource.h>
 * #include <bpf/bpf.h>
 * #include <bpf/libbpf.h>
 * #include "perf-sys.h"
 * #include "trace_helpers.h"
 */
/* Context-Before
 * 		err_exit(error);
 * 	}
 * 
 * 	/* clear stack map */
 * 	while (bpf_map_get_next_key(stack_map, &stackid, &next_id) == 0) {
 * 		bpf_map_delete_elem(stack_map, &next_id);
 * 		stackid = next_id;
 * 	}
 * }
 * 
 * static inline int generate_load(void)
 * {
 * 	if (system("dd if=/dev/zero of=/dev/null count=5000k status=none") < 0) {
 * 		printf("failed to generate some load with dd: %s\n", strerror(errno));
 * 		return -1;
 * 	}
 * 
 * 	return 0;
 * }
 */
static void test_perf_event_all_cpu(struct perf_event_attr *attr)
{
	int nr_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	struct bpf_link **links = calloc(nr_cpus, sizeof(struct bpf_link *));
	int i, pmu_fd, error = 1;

	if (!links) {
		printf("malloc of links failed\n");
		goto err;
	}

	/* system wide perf event, no need to inherit */
	attr->inherit = 0;

	/* open perf_event on all cpus */
	for (i = 0; i < nr_cpus; i++) {
		pmu_fd = sys_perf_event_open(attr, -1, i, -1, 0);
		if (pmu_fd < 0) {
			printf("sys_perf_event_open failed\n");
			goto all_cpu_err;
		}
		links[i] = bpf_program__attach_perf_event(prog, pmu_fd);
		if (libbpf_get_error(links[i])) {
			printf("bpf_program__attach_perf_event failed\n");
			links[i] = NULL;
			close(pmu_fd);
			goto all_cpu_err;
		}
	}

	if (generate_load() < 0)
		goto all_cpu_err;

	print_stacks();
	error = 0;
all_cpu_err:
	for (i--; i >= 0; i--)
		bpf_link__destroy(links[i]);
err:
	free(links);
	if (error)
		err_exit(error);
}
/* Context-After
 * static void test_perf_event_task(struct perf_event_attr *attr)
 * {
 * 	struct bpf_link *link = NULL;
 * 	int pmu_fd, error = 1;
 * 
 * 	/* per task perf event, enable inherit so the "dd ..." command can be traced properly.
 * 	 * Enabling inherit will cause bpf_perf_prog_read_time helper failure.
 * 	 */
 * 	attr->inherit = 1;
 * 
 * 	/* open task bound event */
 * 	pmu_fd = sys_perf_event_open(attr, 0, -1, -1, 0);
 * 	if (pmu_fd < 0) {
 * 		printf("sys_perf_event_open failed\n");
 * 		goto err;
 * 	}
 * 	link = bpf_program__attach_perf_event(prog, pmu_fd);
 * 	if (libbpf_get_error(link)) {
 * 		printf("bpf_program__attach_perf_event failed\n");
 */
