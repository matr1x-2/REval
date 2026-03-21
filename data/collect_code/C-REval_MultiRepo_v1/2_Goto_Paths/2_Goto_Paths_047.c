/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_047 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 11 */
/* NLOC: 52 */
/* Subsystem: tools */
/* Includes
 * #include <test_progs.h>
 * #include <network_helpers.h>
 * #include "test_xdp_context_test_run.skel.h"
 * #include "test_xdp_meta.skel.h"
 */
/* Context-Before
 * 	ret = bpf_xdp_attach(tap_ifindex, bpf_program__fd(xdp_prog),
 * 			     0, NULL);
 * 	if (!ASSERT_GE(ret, 0, "bpf_xdp_attach"))
 * 		goto close;
 * 
 * 	ret = write_test_packet(tap_fd);
 * 	if (!ASSERT_OK(ret, "write_test_packet"))
 * 		goto close;
 * 
 * 	if (!ASSERT_TRUE(*test_pass, "test_pass"))
 * 		dump_err_stream(tc_prio_2_prog ? : tc_prio_1_prog);
 * 
 * close:
 * 	if (tap_fd >= 0)
 * 		close(tap_fd);
 * 	netns_free(ns);
 * }
 * 
 * /* Write a packet to a tap dev and copy it to ingress of a dummy dev */
 */
static void test_tuntap_mirred(struct bpf_program *xdp_prog,
			       struct bpf_program *tc_prog,
			       bool *test_pass)
{
	LIBBPF_OPTS(bpf_tc_hook, tc_hook, .attach_point = BPF_TC_INGRESS);
	LIBBPF_OPTS(bpf_tc_opts, tc_opts, .handle = 1, .priority = 1);
	struct netns_obj *ns = NULL;
	int dummy_ifindex;
	int tap_fd = -1;
	int tap_ifindex;
	int ret;

	*test_pass = false;

	ns = netns_new(TAP_NETNS, true);
	if (!ASSERT_OK_PTR(ns, "netns_new"))
		return;

	/* Setup dummy interface */
	SYS(close, "ip link add name " DUMMY_NAME " type dummy");
	SYS(close, "ip link set dev " DUMMY_NAME " up");

	dummy_ifindex = if_nametoindex(DUMMY_NAME);
	if (!ASSERT_GE(dummy_ifindex, 0, "if_nametoindex"))
		goto close;

	tc_hook.ifindex = dummy_ifindex;
	ret = bpf_tc_hook_create(&tc_hook);
	if (!ASSERT_OK(ret, "bpf_tc_hook_create"))
		goto close;

	tc_opts.prog_fd = bpf_program__fd(tc_prog);
	ret = bpf_tc_attach(&tc_hook, &tc_opts);
	if (!ASSERT_OK(ret, "bpf_tc_attach"))
		goto close;

	/* Setup TAP interface */
	tap_fd = open_tuntap(TAP_NAME, true);
	if (!ASSERT_GE(tap_fd, 0, "open_tuntap"))
		goto close;

	SYS(close, "ip link set dev " TAP_NAME " up");

	tap_ifindex = if_nametoindex(TAP_NAME);
	if (!ASSERT_GE(tap_ifindex, 0, "if_nametoindex"))
		goto close;

	ret = bpf_xdp_attach(tap_ifindex, bpf_program__fd(xdp_prog), 0, NULL);
	if (!ASSERT_GE(ret, 0, "bpf_xdp_attach"))
		goto close;

	/* Copy all packets received from TAP to dummy ingress */
	SYS(close, "tc qdisc add dev " TAP_NAME " clsact");
	SYS(close, "tc filter add dev " TAP_NAME " ingress "
		   "protocol all matchall "
		   "action mirred ingress mirror dev " DUMMY_NAME);

	/* Receive a packet on TAP */
	ret = write_test_packet(tap_fd);
	if (!ASSERT_OK(ret, "write_test_packet"))
		goto close;

	if (!ASSERT_TRUE(*test_pass, "test_pass"))
		dump_err_stream(tc_prog);

close:
	if (tap_fd >= 0)
		close(tap_fd);
	netns_free(ns);
}
/* Context-After
 * void test_xdp_context_tuntap(void)
 * {
 * 	struct test_xdp_meta *skel = NULL;
 * 
 * 	skel = test_xdp_meta__open_and_load();
 * 	if (!ASSERT_OK_PTR(skel, "open and load skeleton"))
 * 		return;
 * 
 * 	if (test__start_subtest("data_meta"))
 * 		test_tuntap(skel->progs.ing_xdp,
 * 			    skel->progs.ing_cls,
 * 			    NULL, /* tc prio 2 */
 * 			    &skel->bss->test_pass);
 * 	if (test__start_subtest("dynptr_read"))
 * 		test_tuntap(skel->progs.ing_xdp,
 * 			    skel->progs.ing_cls_dynptr_read,
 * 			    NULL, /* tc prio 2 */
 * 			    &skel->bss->test_pass);
 * 	if (test__start_subtest("dynptr_slice"))
 */
