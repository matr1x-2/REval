/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_010 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 47 */
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
 * 	printf("Test HW_CACHE_L1D\n");
 * 	test_perf_event_all_cpu(&attr_hw_cache_l1d);
 * 	test_perf_event_task(&attr_hw_cache_l1d);
 * 
 * 	printf("Test HW_CACHE_BPU\n");
 * 	test_perf_event_all_cpu(&attr_hw_cache_branch_miss);
 * 	test_perf_event_task(&attr_hw_cache_branch_miss);
 * 
 * 	printf("Test Instruction Retired\n");
 * 	test_perf_event_all_cpu(&attr_type_raw);
 * 	test_perf_event_task(&attr_type_raw);
 * 
 * 	printf("Test Lock Load\n");
 * 	test_perf_event_all_cpu(&attr_type_raw_lock_load);
 * 	test_perf_event_task(&attr_type_raw_lock_load);
 * 
 * 	printf("*** PASS ***\n");
 * }
 */
int main(int argc, char **argv)
{
	struct bpf_object *obj = NULL;
	char filename[256];
	int error = 1;

	snprintf(filename, sizeof(filename), "%s_kern.o", argv[0]);

	signal(SIGINT, err_exit);
	signal(SIGTERM, err_exit);

	if (load_kallsyms()) {
		printf("failed to process /proc/kallsyms\n");
		goto cleanup;
	}

	obj = bpf_object__open_file(filename, NULL);
	if (libbpf_get_error(obj)) {
		printf("opening BPF object file failed\n");
		obj = NULL;
		goto cleanup;
	}

	prog = bpf_object__find_program_by_name(obj, "bpf_prog1");
	if (!prog) {
		printf("finding a prog in obj file failed\n");
		goto cleanup;
	}

	/* load BPF program */
	if (bpf_object__load(obj)) {
		printf("loading BPF object file failed\n");
		goto cleanup;
	}

	map_fd[0] = bpf_object__find_map_fd_by_name(obj, "counts");
	map_fd[1] = bpf_object__find_map_fd_by_name(obj, "stackmap");
	if (map_fd[0] < 0 || map_fd[1] < 0) {
		printf("finding a counts/stackmap map in obj file failed\n");
		goto cleanup;
	}

	pid = fork();
	if (pid == 0) {
		read_trace_pipe();
		return 0;
	} else if (pid == -1) {
		printf("couldn't spawn process\n");
		goto cleanup;
	}

	test_bpf_perf_event();
	error = 0;

cleanup:
	bpf_object__close(obj);
	err_exit(error);
}
/* Context-After: <empty> */
