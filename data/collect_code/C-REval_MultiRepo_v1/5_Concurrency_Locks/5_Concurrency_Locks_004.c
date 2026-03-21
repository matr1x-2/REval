/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_004 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 21 */
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
 * 		return;
 * 
 * 	info_len = sizeof(info);
 * 	err = bpf_link_get_info_by_fd(bpf_link__fd(link), &info, &info_len);
 * 	ASSERT_OK(err, "bpf_link_get_info_by_fd");
 * 	ASSERT_EQ(info.iter.task.tid, getpid(), "check_task_tid");
 * 
 * 	bpf_link__destroy(link);
 * }
 * 
 * static pthread_mutex_t do_nothing_mutex;
 * 
 * static void *do_nothing_wait(void *arg)
 * {
 * 	pthread_mutex_lock(&do_nothing_mutex);
 * 	pthread_mutex_unlock(&do_nothing_mutex);
 * 
 * 	pthread_exit(arg);
 * }
 */
static void test_task_common_nocheck(struct bpf_iter_attach_opts *opts,
				     int *num_unknown, int *num_known)
{
	struct bpf_iter_tasks *skel;
	pthread_t thread_id;
	void *ret;

	skel = bpf_iter_tasks__open_and_load();
	if (!ASSERT_OK_PTR(skel, "bpf_iter_tasks__open_and_load"))
		return;

	ASSERT_OK(pthread_mutex_lock(&do_nothing_mutex), "pthread_mutex_lock");

	ASSERT_OK(pthread_create(&thread_id, NULL, &do_nothing_wait, NULL),
		  "pthread_create");

	skel->bss->tid = sys_gettid();

	do_dummy_read_opts(skel->progs.dump_task, opts);

	*num_unknown = skel->bss->num_unknown_tid;
	*num_known = skel->bss->num_known_tid;

	ASSERT_OK(pthread_mutex_unlock(&do_nothing_mutex), "pthread_mutex_unlock");
	ASSERT_FALSE(pthread_join(thread_id, &ret) || ret != NULL,
		     "pthread_join");

	bpf_iter_tasks__destroy(skel);
}
/* Context-After
 * static void test_task_common(struct bpf_iter_attach_opts *opts, int num_unknown, int num_known)
 * {
 * 	int num_unknown_tid, num_known_tid;
 * 
 * 	test_task_common_nocheck(opts, &num_unknown_tid, &num_known_tid);
 * 	ASSERT_EQ(num_unknown_tid, num_unknown, "check_num_unknown_tid");
 * 	ASSERT_EQ(num_known_tid, num_known, "check_num_known_tid");
 * }
 * 
 * static void *run_test_task_tid(void *arg)
 * {
 * 	LIBBPF_OPTS(bpf_iter_attach_opts, opts);
 * 	union bpf_iter_link_info linfo;
 * 	int num_unknown_tid, num_known_tid;
 * 
 * 	ASSERT_NEQ(getpid(), sys_gettid(), "check_new_thread_id");
 * 
 * 	memset(&linfo, 0, sizeof(linfo));
 * 	linfo.task.tid = sys_gettid();
 */
