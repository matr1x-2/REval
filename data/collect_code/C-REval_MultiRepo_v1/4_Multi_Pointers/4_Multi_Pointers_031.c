/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_031 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 12 */
/* NLOC: 49 */
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
 * 		return NULL;
 * 	return xfs_perag_get(mp, xfs_daddr_to_agno(mp, map->bm_bn));
 * }
 * 
 * static inline struct xfs_buf_cache *
 * xfs_buftarg_buf_cache(
 * 	struct xfs_buftarg		*btp,
 * 	struct xfs_perag		*pag)
 * {
 * 	if (pag)
 * 		return &pag->pag_bcache;
 * 	return btp->bt_cache;
 * }
 * 
 * /*
 *  * Assembles a buffer covering the specified range. The code is optimised for
 *  * cache hits, as metadata intensive workloads will see 3 orders of magnitude
 *  * more hits than misses.
 *  */
 * int
 */
xfs_buf_get_map(
	struct xfs_buftarg	*btp,
	struct xfs_buf_map	*map,
	int			nmaps,
	xfs_buf_flags_t		flags,
	struct xfs_buf		**bpp)
{
	struct xfs_buf_cache	*bch;
	struct xfs_perag	*pag;
	struct xfs_buf		*bp = NULL;
	struct xfs_buf_map	cmap = { .bm_bn = map[0].bm_bn };
	int			error;
	int			i;

	if (flags & XBF_LIVESCAN)
		cmap.bm_flags |= XBM_LIVESCAN;
	for (i = 0; i < nmaps; i++)
		cmap.bm_len += map[i].bm_len;

	error = xfs_buf_map_verify(btp, &cmap);
	if (error)
		return error;

	pag = xfs_buftarg_get_pag(btp, &cmap);
	bch = xfs_buftarg_buf_cache(btp, pag);

	error = xfs_buf_lookup(bch, &cmap, flags, &bp);
	if (error && error != -ENOENT)
		goto out_put_perag;

	/* cache hits always outnumber misses by at least 10:1 */
	if (unlikely(!bp)) {
		XFS_STATS_INC(btp->bt_mount, xb_miss_locked);

		if (flags & XBF_INCORE)
			goto out_put_perag;

		/* xfs_buf_find_insert() consumes the perag reference. */
		error = xfs_buf_find_insert(btp, bch, pag, &cmap, map, nmaps,
				flags, &bp);
		if (error)
			return error;
	} else {
		XFS_STATS_INC(btp->bt_mount, xb_get_locked);
		if (pag)
			xfs_perag_put(pag);
	}

	/*
	 * Clear b_error if this is a lookup from a caller that doesn't expect
	 * valid data to be found in the buffer.
	 */
	if (!(flags & XBF_READ))
		xfs_buf_ioerror(bp, 0);

	XFS_STATS_INC(btp->bt_mount, xb_get);
	trace_xfs_buf_get(bp, flags, _RET_IP_);
	*bpp = bp;
	return 0;

out_put_perag:
	if (pag)
		xfs_perag_put(pag);
	return error;
}
/* Context-After
 * int
 * _xfs_buf_read(
 * 	struct xfs_buf		*bp)
 * {
 * 	ASSERT(bp->b_maps[0].bm_bn != XFS_BUF_DADDR_NULL);
 * 
 * 	bp->b_flags &= ~(XBF_WRITE | XBF_ASYNC | XBF_READ_AHEAD | XBF_DONE);
 * 	bp->b_flags |= XBF_READ;
 * 	xfs_buf_submit(bp);
 * 	return xfs_buf_iowait(bp);
 * }
 * 
 * /*
 *  * Reverify a buffer found in cache without an attached ->b_ops.
 *  *
 *  * If the caller passed an ops structure and the buffer doesn't have ops
 *  * assigned, set the ops and use it to verify the contents. If verification
 *  * fails, clear XBF_DONE. We assume the buffer has no recorded errors and is
 *  * already in XBF_DONE state on entry.
 */
