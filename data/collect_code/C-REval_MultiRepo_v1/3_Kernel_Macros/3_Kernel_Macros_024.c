/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_024 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 25 */
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
 * 			continue;
 * 		}
 * 
 * 		if (subflow->backup || subflow->request_bkup) {
 * 			if (!backup)
 * 				backup = ssk;
 * 			continue;
 * 		}
 * 
 * 		if (!pick)
 * 			pick = ssk;
 * 	}
 * 
 * 	if (pick)
 * 		return pick;
 * 
 * 	/* use backup only if there are no progresses anywhere */
 * 	return min_stale_count > 1 ? backup : NULL;
 * }
 */
bool __mptcp_retransmit_pending_data(struct sock *sk)
{
	struct mptcp_data_frag *cur, *rtx_head;
	struct mptcp_sock *msk = mptcp_sk(sk);

	if (__mptcp_check_fallback(msk))
		return false;

	/* the closing socket has some data untransmitted and/or unacked:
	 * some data in the mptcp rtx queue has not really xmitted yet.
	 * keep it simple and re-inject the whole mptcp level rtx queue
	 */
	mptcp_data_lock(sk);
	__mptcp_clean_una_wakeup(sk);
	rtx_head = mptcp_rtx_head(sk);
	if (!rtx_head) {
		mptcp_data_unlock(sk);
		return false;
	}

	msk->recovery_snd_nxt = msk->snd_nxt;
	msk->recovery = true;
	mptcp_data_unlock(sk);

	msk->first_pending = rtx_head;
	msk->snd_burst = 0;

	/* be sure to clear the "sent status" on all re-injected fragments */
	list_for_each_entry(cur, &msk->rtx_queue, list) {
		if (!cur->already_sent)
			break;
		cur->already_sent = 0;
	}

	return true;
}
/* Context-After
 * /* flags for __mptcp_close_ssk() */
 * #define MPTCP_CF_PUSH		BIT(1)
 * 
 * /* be sure to send a reset only if the caller asked for it, also
 *  * clean completely the subflow status when the subflow reaches
 *  * TCP_CLOSE state
 *  */
 * static void __mptcp_subflow_disconnect(struct sock *ssk,
 * 				       struct mptcp_subflow_context *subflow,
 * 				       bool fastclosing)
 * {
 * 	if (((1 << ssk->sk_state) & (TCPF_CLOSE | TCPF_LISTEN)) ||
 * 	    fastclosing) {
 * 		/* The MPTCP code never wait on the subflow sockets, TCP-level
 * 		 * disconnect should never fail
 * 		 */
 * 		WARN_ON_ONCE(tcp_disconnect(ssk, 0));
 * 		mptcp_subflow_ctx_reset(subflow);
 * 	} else {
 */
