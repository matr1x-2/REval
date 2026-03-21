/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_039 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 27 */
/* NLOC: 85 */
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
 * {
 * 	const struct mptcp_sock *msk = mptcp_sk(sk);
 * 	const struct sk_buff *skb;
 * 
 * 	skb = skb_peek(&sk->sk_receive_queue);
 * 	if (skb) {
 * 		u64 hint_val = READ_ONCE(msk->ack_seq) - MPTCP_SKB_CB(skb)->map_seq;
 * 
 * 		if (hint_val >= INT_MAX)
 * 			return INT_MAX;
 * 
 * 		return (unsigned int)hint_val;
 * 	}
 * 
 * 	if (sk->sk_state == TCP_CLOSE || (sk->sk_shutdown & RCV_SHUTDOWN))
 * 		return 1;
 * 
 * 	return 0;
 * }
 */
static int mptcp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len,
			 int flags, int *addr_len)
{
	struct mptcp_sock *msk = mptcp_sk(sk);
	struct scm_timestamping_internal tss;
	int copied = 0, cmsg_flags = 0;
	int target;
	long timeo;

	/* MSG_ERRQUEUE is really a no-op till we support IP_RECVERR */
	if (unlikely(flags & MSG_ERRQUEUE))
		return inet_recv_error(sk, msg, len, addr_len);

	lock_sock(sk);
	if (unlikely(sk->sk_state == TCP_LISTEN)) {
		copied = -ENOTCONN;
		goto out_err;
	}

	mptcp_rps_record_subflows(msk);

	timeo = sock_rcvtimeo(sk, flags & MSG_DONTWAIT);

	len = min_t(size_t, len, INT_MAX);
	target = sock_rcvlowat(sk, flags & MSG_WAITALL, len);

	if (unlikely(msk->recvmsg_inq))
		cmsg_flags = MPTCP_CMSG_INQ;

	while (copied < len) {
		int err, bytes_read;

		bytes_read = __mptcp_recvmsg_mskq(sk, msg, len - copied, flags,
						  copied, &tss, &cmsg_flags);
		if (unlikely(bytes_read < 0)) {
			if (!copied)
				copied = bytes_read;
			goto out_err;
		}

		copied += bytes_read;

		if (!list_empty(&msk->backlog_list) && mptcp_move_skbs(sk))
			continue;

		/* only the MPTCP socket status is relevant here. The exit
		 * conditions mirror closely tcp_recvmsg()
		 */
		if (copied >= target)
			break;

		if (copied) {
			if (sk->sk_err ||
			    sk->sk_state == TCP_CLOSE ||
			    (sk->sk_shutdown & RCV_SHUTDOWN) ||
			    !timeo ||
			    signal_pending(current))
				break;
		} else {
			if (sk->sk_err) {
				copied = sock_error(sk);
				break;
			}

			if (sk->sk_shutdown & RCV_SHUTDOWN)
				break;

			if (sk->sk_state == TCP_CLOSE) {
				copied = -ENOTCONN;
				break;
			}

			if (!timeo) {
				copied = -EAGAIN;
				break;
			}

			if (signal_pending(current)) {
				copied = sock_intr_errno(timeo);
				break;
			}
		}

		pr_debug("block timeout %ld\n", timeo);
		mptcp_cleanup_rbuf(msk, copied);
		err = sk_wait_data(sk, &timeo, NULL);
		if (err < 0) {
			err = copied ? : err;
			goto out_err;
		}
	}

	mptcp_cleanup_rbuf(msk, copied);

out_err:
	if (cmsg_flags && copied >= 0) {
		if (cmsg_flags & MPTCP_CMSG_TS)
			tcp_recv_timestamp(msg, sk, &tss);

		if (cmsg_flags & MPTCP_CMSG_INQ) {
			unsigned int inq = mptcp_inq_hint(sk);

			put_cmsg(msg, SOL_TCP, TCP_CM_INQ, sizeof(inq), &inq);
		}
	}

	pr_debug("msk=%p rx queue empty=%d copied=%d\n",
		 msk, skb_queue_empty(&sk->sk_receive_queue), copied);

	release_sock(sk);
	return copied;
}
/* Context-After
 * static void mptcp_retransmit_timer(struct timer_list *t)
 * {
 * 	struct sock *sk = timer_container_of(sk, t, mptcp_retransmit_timer);
 * 	struct mptcp_sock *msk = mptcp_sk(sk);
 * 
 * 	bh_lock_sock(sk);
 * 	if (!sock_owned_by_user(sk)) {
 * 		/* we need a process context to retransmit */
 * 		if (!test_and_set_bit(MPTCP_WORK_RTX, &msk->flags))
 * 			mptcp_schedule_work(sk);
 * 	} else {
 * 		/* delegate our work to tcp_release_cb() */
 * 		__set_bit(MPTCP_RETRANSMIT, &msk->cb_flags);
 * 	}
 * 	bh_unlock_sock(sk);
 * 	sock_put(sk);
 * }
 * 
 * static void mptcp_tout_timer(struct timer_list *t)
 */
