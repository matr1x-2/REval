/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_015 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 47 */
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
 * 		;
 * 	if (!ASSERT_GE(len, 0, "read"))
 * 		goto close_iter;
 * 
 * 	/* test results */
 * 	if (!ASSERT_EQ(skel->bss->key_sum, expected_key, "key_sum"))
 * 		goto close_iter;
 * 	if (!ASSERT_EQ(skel->bss->val_sum, expected_val, "val_sum"))
 * 		goto close_iter;
 * 
 * close_iter:
 * 	close(iter_fd);
 * free_link:
 * 	bpf_link__destroy(link);
 * out:
 * 	bpf_iter_bpf_percpu_array_map__destroy(skel);
 * 	free(val);
 * }
 * 
 * /* An iterator program deletes all local storage in a map. */
 */
static void test_bpf_sk_storage_delete(void)
{
	DECLARE_LIBBPF_OPTS(bpf_iter_attach_opts, opts);
	struct bpf_iter_bpf_sk_storage_helpers *skel;
	union bpf_iter_link_info linfo;
	int err, len, map_fd, iter_fd;
	struct bpf_link *link;
	int sock_fd = -1;
	__u32 val = 42;
	char buf[64];

	skel = bpf_iter_bpf_sk_storage_helpers__open_and_load();
	if (!ASSERT_OK_PTR(skel, "bpf_iter_bpf_sk_storage_helpers__open_and_load"))
		return;

	map_fd = bpf_map__fd(skel->maps.sk_stg_map);

	sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
	if (!ASSERT_GE(sock_fd, 0, "socket"))
		goto out;

	err = bpf_map_update_elem(map_fd, &sock_fd, &val, BPF_NOEXIST);
	if (!ASSERT_OK(err, "map_update"))
		goto out;

	memset(&linfo, 0, sizeof(linfo));
	linfo.map.map_fd = map_fd;
	opts.link_info = &linfo;
	opts.link_info_len = sizeof(linfo);
	link = bpf_program__attach_iter(skel->progs.delete_bpf_sk_storage_map,
					&opts);
	if (!ASSERT_OK_PTR(link, "attach_iter"))
		goto out;

	iter_fd = bpf_iter_create(bpf_link__fd(link));
	if (!ASSERT_GE(iter_fd, 0, "create_iter"))
		goto free_link;

	/* do some tests */
	while ((len = read(iter_fd, buf, sizeof(buf))) > 0)
		;
	if (!ASSERT_GE(len, 0, "read"))
		goto close_iter;

	/* test results */
	err = bpf_map_lookup_elem(map_fd, &sock_fd, &val);

	 /* Note: The following assertions serve to ensure
	  * the value was deleted. It does so by asserting
	  * that bpf_map_lookup_elem has failed. This might
	  * seem counterintuitive at first.
	  */
	ASSERT_ERR(err, "bpf_map_lookup_elem");
	ASSERT_EQ(errno, ENOENT, "bpf_map_lookup_elem");

close_iter:
	close(iter_fd);
free_link:
	bpf_link__destroy(link);
out:
	if (sock_fd >= 0)
		close(sock_fd);
	bpf_iter_bpf_sk_storage_helpers__destroy(skel);
}
/* Context-After
 * /* This creates a socket and its local storage. It then runs a task_iter BPF
 *  * program that replaces the existing socket local storage with the tgid of the
 *  * only task owning a file descriptor to this socket, this process, prog_tests.
 *  * It then runs a tcp socket iterator that negates the value in the existing
 *  * socket local storage, the test verifies that the resulting value is -pid.
 *  */
 * static void test_bpf_sk_storage_get(void)
 * {
 * 	struct bpf_iter_bpf_sk_storage_helpers *skel;
 * 	int err, map_fd, val = -1;
 * 	int sock_fd = -1;
 * 
 * 	skel = bpf_iter_bpf_sk_storage_helpers__open_and_load();
 * 	if (!ASSERT_OK_PTR(skel, "bpf_iter_bpf_sk_storage_helpers__open_and_load"))
 * 		return;
 * 
 * 	sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
 * 	if (!ASSERT_GE(sock_fd, 0, "socket"))
 * 		goto out;
 */
