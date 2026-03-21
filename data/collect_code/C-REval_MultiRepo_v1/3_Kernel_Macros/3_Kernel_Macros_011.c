/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_011 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 20 */
/* Subsystem: fs */
/* Includes
 * #include "xfs_platform.h"
 * #include <linux/backing-dev.h>
 * #include <linux/dax.h>
 * #include "xfs_shared.h"
 * #include "xfs_format.h"
 * #include "xfs_log_format.h"
 * #include "xfs_trans_resv.h"
 * #include "xfs_mount.h"
 * #include "xfs_trace.h"
 * #include "xfs_log.h"
 * #include "xfs_log_recover.h"
 * #include "xfs_log_priv.h"
 * #include "xfs_trans.h"
 * #include "xfs_buf_item.h"
 * #include "xfs_errortag.h"
 * #include "xfs_error.h"
 * #include "xfs_ag.h"
 * #include "xfs_buf_mem.h"
 * #include "xfs_notify_failure.h"
 */
/* Context-Before
 * __xfs_buf_mark_corrupt(
 * 	struct xfs_buf		*bp,
 * 	xfs_failaddr_t		fa)
 * {
 * 	ASSERT(bp->b_flags & XBF_DONE);
 * 
 * 	xfs_buf_corruption_error(bp, fa);
 * 	xfs_buf_stale(bp);
 * }
 * 
 * /*
 *  *	Handling of buffer targets (buftargs).
 *  */
 * 
 * /*
 *  * Wait for any bufs with callbacks that have been submitted but have not yet
 *  * returned. These buffers will have an elevated hold count, so wait on those
 *  * while freeing all the buffers only held by the LRU.
 *  */
 * static enum lru_status
 */
xfs_buftarg_drain_rele(
	struct list_head	*item,
	struct list_lru_one	*lru,
	void			*arg)

{
	struct xfs_buf		*bp = container_of(item, struct xfs_buf, b_lru);
	struct list_head	*dispose = arg;

	if (!spin_trylock(&bp->b_lock))
		return LRU_SKIP;
	if (bp->b_hold > 1) {
		/* need to wait, so skip it this pass */
		spin_unlock(&bp->b_lock);
		trace_xfs_buf_drain_buftarg(bp, _RET_IP_);
		return LRU_SKIP;
	}

	/*
	 * clear the LRU reference count so the buffer doesn't get
	 * ignored in xfs_buf_rele().
	 */
	atomic_set(&bp->b_lru_ref, 0);
	bp->b_state |= XFS_BSTATE_DISPOSE;
	list_lru_isolate_move(lru, item, dispose);
	spin_unlock(&bp->b_lock);
	return LRU_REMOVED;
}
/* Context-After
 * /*
 *  * Wait for outstanding I/O on the buftarg to complete.
 *  */
 * void
 * xfs_buftarg_wait(
 * 	struct xfs_buftarg	*btp)
 * {
 * 	/*
 * 	 * First wait for all in-flight readahead buffers to be released.  This is
 * 	 * critical as new buffers do not make the LRU until they are released.
 * 	 *
 * 	 * Next, flush the buffer workqueue to ensure all completion processing
 * 	 * has finished. Just waiting on buffer locks is not sufficient for
 * 	 * async IO as the reference count held over IO is not released until
 * 	 * after the buffer lock is dropped. Hence we need to ensure here that
 * 	 * all reference counts have been dropped before we start walking the
 * 	 * LRU list.
 * 	 */
 * 	while (percpu_counter_sum(&btp->bt_readahead_count))
 */
