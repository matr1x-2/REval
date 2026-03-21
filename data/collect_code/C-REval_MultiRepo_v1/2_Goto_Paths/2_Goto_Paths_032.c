/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_032 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 21 */
/* NLOC: 93 */
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
 * 	if (not_sent >= limit)
 * 		return 0;
 * 
 * 	return limit - not_sent;
 * }
 * 
 * static void mptcp_rps_record_subflows(const struct mptcp_sock *msk)
 * {
 * 	struct mptcp_subflow_context *subflow;
 * 
 * 	if (!rfs_is_needed())
 * 		return;
 * 
 * 	mptcp_for_each_subflow(msk, subflow) {
 * 		struct sock *ssk = mptcp_subflow_tcp_sock(subflow);
 * 
 * 		sock_rps_record_flow(ssk);
 * 	}
 * }
 */
static int mptcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t len)
{
	struct mptcp_sock *msk = mptcp_sk(sk);
	struct page_frag *pfrag;
	size_t copied = 0;
	int ret = 0;
	long timeo;

	/* silently ignore everything else */
	msg->msg_flags &= MSG_MORE | MSG_DONTWAIT | MSG_NOSIGNAL | MSG_FASTOPEN;

	lock_sock(sk);

	mptcp_rps_record_subflows(msk);

	if (unlikely(inet_test_bit(DEFER_CONNECT, sk) ||
		     msg->msg_flags & MSG_FASTOPEN)) {
		int copied_syn = 0;

		ret = mptcp_sendmsg_fastopen(sk, msg, len, &copied_syn);
		copied += copied_syn;
		if (ret == -EINPROGRESS && copied_syn > 0)
			goto out;
		else if (ret)
			goto do_error;
	}

	timeo = sock_sndtimeo(sk, msg->msg_flags & MSG_DONTWAIT);

	if ((1 << sk->sk_state) & ~(TCPF_ESTABLISHED | TCPF_CLOSE_WAIT)) {
		ret = sk_stream_wait_connect(sk, &timeo);
		if (ret)
			goto do_error;
	}

	ret = -EPIPE;
	if (unlikely(sk->sk_err || (sk->sk_shutdown & SEND_SHUTDOWN)))
		goto do_error;

	pfrag = sk_page_frag(sk);

	while (msg_data_left(msg)) {
		int total_ts, frag_truesize = 0;
		struct mptcp_data_frag *dfrag;
		bool dfrag_collapsed;
		size_t psize, offset;
		u32 copy_limit;

		/* ensure fitting the notsent_lowat() constraint */
		copy_limit = mptcp_send_limit(sk);
		if (!copy_limit)
			goto wait_for_memory;

		/* reuse tail pfrag, if possible, or carve a new one from the
		 * page allocator
		 */
		dfrag = mptcp_pending_tail(sk);
		dfrag_collapsed = mptcp_frag_can_collapse_to(msk, pfrag, dfrag);
		if (!dfrag_collapsed) {
			if (!mptcp_page_frag_refill(sk, pfrag))
				goto wait_for_memory;

			dfrag = mptcp_carve_data_frag(msk, pfrag, pfrag->offset);
			frag_truesize = dfrag->overhead;
		}

		/* we do not bound vs wspace, to allow a single packet.
		 * memory accounting will prevent execessive memory usage
		 * anyway
		 */
		offset = dfrag->offset + dfrag->data_len;
		psize = pfrag->size - offset;
		psize = min_t(size_t, psize, msg_data_left(msg));
		psize = min_t(size_t, psize, copy_limit);
		total_ts = psize + frag_truesize;

		if (!sk_wmem_schedule(sk, total_ts))
			goto wait_for_memory;

		ret = do_copy_data_nocache(sk, psize, &msg->msg_iter,
					   page_address(dfrag->page) + offset);
		if (ret)
			goto do_error;

		/* data successfully copied into the write queue */
		sk_forward_alloc_add(sk, -total_ts);
		copied += psize;
		dfrag->data_len += psize;
		frag_truesize += psize;
		pfrag->offset += frag_truesize;
		WRITE_ONCE(msk->write_seq, msk->write_seq + psize);

		/* charge data on mptcp pending queue to the msk socket
		 * Note: we charge such data both to sk and ssk
		 */
		sk_wmem_queued_add(sk, frag_truesize);
		if (!dfrag_collapsed) {
			get_page(dfrag->page);
			list_add_tail(&dfrag->list, &msk->rtx_queue);
			if (!msk->first_pending)
				msk->first_pending = dfrag;
		}
		pr_debug("msk=%p dfrag at seq=%llu len=%u sent=%u new=%d\n", msk,
			 dfrag->data_seq, dfrag->data_len, dfrag->already_sent,
			 !dfrag_collapsed);

		continue;

wait_for_memory:
		set_bit(SOCK_NOSPACE, &sk->sk_socket->flags);
		__mptcp_push_pending(sk, msg->msg_flags);
		ret = sk_stream_wait_memory(sk, &timeo);
		if (ret)
			goto do_error;
	}

	if (copied)
		__mptcp_push_pending(sk, msg->msg_flags);

out:
	release_sock(sk);
	return copied;

do_error:
	if (copied)
		goto out;

	copied = sk_stream_error(sk, msg->msg_flags, ret);
	goto out;
}
/* Context-After
 * static void mptcp_rcv_space_adjust(struct mptcp_sock *msk, int copied);
 * 
 * static void mptcp_eat_recv_skb(struct sock *sk, struct sk_buff *skb)
 * {
 * 	/* avoid the indirect call, we know the destructor is sock_rfree */
 * 	skb->destructor = NULL;
 * 	skb->sk = NULL;
 * 	atomic_sub(skb->truesize, &sk->sk_rmem_alloc);
 * 	sk_mem_uncharge(sk, skb->truesize);
 * 	__skb_unlink(skb, &sk->sk_receive_queue);
 * 	skb_attempt_defer_free(skb);
 * }
 * 
 * static int __mptcp_recvmsg_mskq(struct sock *sk, struct msghdr *msg,
 * 				size_t len, int flags, int copied_total,
 * 				struct scm_timestamping_internal *tss,
 * 				int *cmsg_flags)
 * {
 * 	struct mptcp_sock *msk = mptcp_sk(sk);
 */
