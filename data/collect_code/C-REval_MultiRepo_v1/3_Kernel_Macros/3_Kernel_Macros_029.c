/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_029 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 23 */
/* Subsystem: net */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/module.h>
 * #include <linux/netdevice.h>
 * #include <linux/sched/signal.h>
 * #include <linux/atomic.h>
 * #include <net/aligned_data.h>
 * #include <net/rps.h>
 * #include <net/sock.h>
 * #include <net/inet_common.h>
 * #include <net/inet_hashtables.h>
 * #include <net/protocol.h>
 * #include <net/tcp_states.h>
 * #include <net/transp_v6.h>
 * #include <net/mptcp.h>
 * #include <net/hotdata.h>
 * #include <net/xfrm.h>
 * #include <asm/ioctls.h>
 * #include "protocol.h"
 * #include "mib.h"
 * #include <trace/events/mptcp.h>
 */
/* Context-Before
 * 	.listen		   = mptcp_listen,
 * 	.shutdown	   = inet_shutdown,
 * 	.setsockopt	   = sock_common_setsockopt,
 * 	.getsockopt	   = sock_common_getsockopt,
 * 	.sendmsg	   = inet_sendmsg,
 * 	.recvmsg	   = inet_recvmsg,
 * 	.mmap		   = sock_no_mmap,
 * 	.set_rcvlowat	   = mptcp_set_rcvlowat,
 * 	.read_sock	   = mptcp_read_sock,
 * 	.splice_read	   = mptcp_splice_read,
 * };
 * 
 * static struct inet_protosw mptcp_protosw = {
 * 	.type		= SOCK_STREAM,
 * 	.protocol	= IPPROTO_MPTCP,
 * 	.prot		= &mptcp_prot,
 * 	.ops		= &mptcp_stream_ops,
 * 	.flags		= INET_PROTOSW_ICSK,
 * };
 */
static int mptcp_napi_poll(struct napi_struct *napi, int budget)
{
	struct mptcp_delegated_action *delegated;
	struct mptcp_subflow_context *subflow;
	int work_done = 0;

	delegated = container_of(napi, struct mptcp_delegated_action, napi);
	while ((subflow = mptcp_subflow_delegated_next(delegated)) != NULL) {
		struct sock *ssk = mptcp_subflow_tcp_sock(subflow);

		bh_lock_sock_nested(ssk);
		if (!sock_owned_by_user(ssk)) {
			mptcp_subflow_process_delegated(ssk, xchg(&subflow->delegated_status, 0));
		} else {
			/* tcp_release_cb_override already processed
			 * the action or will do at next release_sock().
			 * In both case must dequeue the subflow here - on the same
			 * CPU that scheduled it.
			 */
			smp_wmb();
			clear_bit(MPTCP_DELEGATE_SCHEDULED, &subflow->delegated_status);
		}
		bh_unlock_sock(ssk);
		sock_put(ssk);

		if (++work_done == budget)
			return budget;
	}

	/* always provide a 0 'work_done' argument, so that napi_complete_done
	 * will not try accessing the NULL napi->dev ptr
	 */
	napi_complete_done(napi, 0);
	return work_done;
}
/* Context-After
 * void __init mptcp_proto_init(void)
 * {
 * 	struct mptcp_delegated_action *delegated;
 * 	int cpu;
 * 
 * 	mptcp_prot.h.hashinfo = tcp_prot.h.hashinfo;
 * 
 * 	if (percpu_counter_init(&mptcp_sockets_allocated, 0, GFP_KERNEL))
 * 		panic("Failed to allocate MPTCP pcpu counter\n");
 * 
 * 	mptcp_napi_dev = alloc_netdev_dummy(0);
 * 	if (!mptcp_napi_dev)
 * 		panic("Failed to allocate MPTCP dummy netdev\n");
 * 	for_each_possible_cpu(cpu) {
 * 		delegated = per_cpu_ptr(&mptcp_delegated_actions, cpu);
 * 		INIT_LIST_HEAD(&delegated->head);
 * 		netif_napi_add_tx(mptcp_napi_dev, &delegated->napi,
 * 				  mptcp_napi_poll);
 * 		napi_enable(&delegated->napi);
 */
