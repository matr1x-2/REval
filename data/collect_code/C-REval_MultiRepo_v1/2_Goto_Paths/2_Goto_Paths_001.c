/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_001 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 12 */
/* NLOC: 53 */
/* Subsystem: tools */
/* Includes
 * #include <test_progs.h>
 * #include <unistd.h>
 * #include <sys/syscall.h>
 * #include <task_local_storage_helpers.h>
 * #include "bpf_iter_ipv6_route.skel.h"
 * #include "bpf_iter_netlink.skel.h"
 * #include "bpf_iter_bpf_map.skel.h"
 * #include "bpf_iter_tasks.skel.h"
 * #include "bpf_iter_task_stack.skel.h"
 * #include "bpf_iter_task_file.skel.h"
 * #include "bpf_iter_task_vmas.skel.h"
 * #include "bpf_iter_task_btf.skel.h"
 * #include "bpf_iter_tcp4.skel.h"
 * #include "bpf_iter_tcp6.skel.h"
 * #include "bpf_iter_udp4.skel.h"
 * #include "bpf_iter_udp6.skel.h"
 * #include "bpf_iter_unix.skel.h"
 * #include "bpf_iter_vma_offset.skel.h"
 * #include "bpf_iter_test_kern1.skel.h"
 * #include "bpf_iter_test_kern2.skel.h"
 */
/* Context-Before
 * 	if (!ASSERT_GE(proc_maps_fd, 0, "open_proc_maps"))
 * 		goto out;
 * 	err = read_fd_into_buffer(proc_maps_fd, proc_maps_output, CMP_BUFFER_SIZE);
 * 	if (!ASSERT_GE(err, 0, "read_prog_maps_fd"))
 * 		goto out;
 * 
 * 	/* strip and compare the first line of the two files */
 * 	str_strip_first_line(task_vma_output);
 * 	str_strip_first_line(proc_maps_output);
 * 
 * 	ASSERT_STREQ(task_vma_output, proc_maps_output, "compare_output");
 * 
 * 	check_bpf_link_info(skel->progs.proc_maps);
 * 
 * out:
 * 	close(proc_maps_fd);
 * 	close(iter_fd);
 * 	bpf_iter_task_vmas__destroy(skel);
 * }
 */
static void test_task_vma_dead_task(void)
{
	struct bpf_iter_task_vmas *skel;
	int wstatus, child_pid = -1;
	time_t start_tm, cur_tm;
	int err, iter_fd = -1;
	int wait_sec = 3;

	skel = bpf_iter_task_vmas__open();
	if (!ASSERT_OK_PTR(skel, "bpf_iter_task_vmas__open"))
		return;

	skel->bss->pid = getpid();

	err = bpf_iter_task_vmas__load(skel);
	if (!ASSERT_OK(err, "bpf_iter_task_vmas__load"))
		goto out;

	skel->links.proc_maps = bpf_program__attach_iter(
		skel->progs.proc_maps, NULL);

	if (!ASSERT_OK_PTR(skel->links.proc_maps, "bpf_program__attach_iter")) {
		skel->links.proc_maps = NULL;
		goto out;
	}

	start_tm = time(NULL);
	cur_tm = start_tm;

	child_pid = fork();
	if (child_pid == 0) {
		/* Fork short-lived processes in the background. */
		while (cur_tm < start_tm + wait_sec) {
			system("echo > /dev/null");
			cur_tm = time(NULL);
		}
		exit(0);
	}

	if (!ASSERT_GE(child_pid, 0, "fork_child"))
		goto out;

	while (cur_tm < start_tm + wait_sec) {
		iter_fd = bpf_iter_create(bpf_link__fd(skel->links.proc_maps));
		if (!ASSERT_GE(iter_fd, 0, "create_iter"))
			goto out;

		/* Drain all data from iter_fd. */
		while (cur_tm < start_tm + wait_sec) {
			err = read_fd_into_buffer(iter_fd, task_vma_output, CMP_BUFFER_SIZE);
			if (!ASSERT_GE(err, 0, "read_iter_fd"))
				goto out;

			cur_tm = time(NULL);

			if (err == 0)
				break;
		}

		close(iter_fd);
		iter_fd = -1;
	}

	check_bpf_link_info(skel->progs.proc_maps);

out:
	waitpid(child_pid, &wstatus, 0);
	close(iter_fd);
	bpf_iter_task_vmas__destroy(skel);
}
/* Context-After
 * void test_bpf_sockmap_map_iter_fd(void)
 * {
 * 	struct bpf_iter_sockmap *skel;
 * 
 * 	skel = bpf_iter_sockmap__open_and_load();
 * 	if (!ASSERT_OK_PTR(skel, "bpf_iter_sockmap__open_and_load"))
 * 		return;
 * 
 * 	do_read_map_iter_fd(&skel->skeleton, skel->progs.copy, skel->maps.sockmap);
 * 
 * 	bpf_iter_sockmap__destroy(skel);
 * }
 * 
 * static void test_task_vma(void)
 * {
 * 	LIBBPF_OPTS(bpf_iter_attach_opts, opts);
 * 	union bpf_iter_link_info linfo;
 * 
 * 	memset(&linfo, 0, sizeof(linfo));
 */
