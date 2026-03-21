/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_011 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 2 */
/* NLOC: 33 */
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
 * 	close(data_pipe[0]);
 * 	close(finish_pipe[1]);
 * }
 * 
 * static void test_task_stack(void)
 * {
 * 	struct bpf_iter_task_stack *skel;
 * 
 * 	skel = bpf_iter_task_stack__open_and_load();
 * 	if (!ASSERT_OK_PTR(skel, "bpf_iter_task_stack__open_and_load"))
 * 		return;
 * 
 * 	do_dummy_read(skel->progs.dump_task_stack);
 * 	do_dummy_read(skel->progs.get_task_user_stacks);
 * 
 * 	ASSERT_EQ(skel->bss->num_user_stacks, 1, "num_user_stacks");
 * 
 * 	bpf_iter_task_stack__destroy(skel);
 * }
 */
static void test_task_file(void)
{
	LIBBPF_OPTS(bpf_iter_attach_opts, opts);
	struct bpf_iter_task_file *skel;
	union bpf_iter_link_info linfo;
	pthread_t thread_id;
	void *ret;

	skel = bpf_iter_task_file__open_and_load();
	if (!ASSERT_OK_PTR(skel, "bpf_iter_task_file__open_and_load"))
		return;

	skel->bss->tgid = getpid();

	ASSERT_OK(pthread_mutex_lock(&do_nothing_mutex), "pthread_mutex_lock");

	ASSERT_OK(pthread_create(&thread_id, NULL, &do_nothing_wait, NULL),
		  "pthread_create");

	memset(&linfo, 0, sizeof(linfo));
	linfo.task.tid = getpid();
	opts.link_info = &linfo;
	opts.link_info_len = sizeof(linfo);

	do_dummy_read_opts(skel->progs.dump_task_file, &opts);

	ASSERT_EQ(skel->bss->count, 0, "check_count");
	ASSERT_EQ(skel->bss->unique_tgid_count, 1, "check_unique_tgid_count");

	skel->bss->last_tgid = 0;
	skel->bss->count = 0;
	skel->bss->unique_tgid_count = 0;

	do_dummy_read(skel->progs.dump_task_file);

	ASSERT_EQ(skel->bss->count, 0, "check_count");
	ASSERT_GT(skel->bss->unique_tgid_count, 1, "check_unique_tgid_count");

	check_bpf_link_info(skel->progs.dump_task_file);

	ASSERT_OK(pthread_mutex_unlock(&do_nothing_mutex), "pthread_mutex_unlock");
	ASSERT_OK(pthread_join(thread_id, &ret), "pthread_join");
	ASSERT_NULL(ret, "pthread_join");

	bpf_iter_task_file__destroy(skel);
}
/* Context-After
 * #define TASKBUFSZ		32768
 * 
 * static char taskbuf[TASKBUFSZ];
 * 
 * static int do_btf_read(struct bpf_iter_task_btf *skel)
 * {
 * 	struct bpf_program *prog = skel->progs.dump_task_struct;
 * 	struct bpf_iter_task_btf__bss *bss = skel->bss;
 * 	int iter_fd = -1, err;
 * 	struct bpf_link *link;
 * 	char *buf = taskbuf;
 * 	int ret = 0;
 * 
 * 	link = bpf_program__attach_iter(prog, NULL);
 * 	if (!ASSERT_OK_PTR(link, "attach_iter"))
 * 		return ret;
 * 
 * 	iter_fd = bpf_iter_create(bpf_link__fd(link));
 * 	if (!ASSERT_GE(iter_fd, 0, "create_iter"))
 */
