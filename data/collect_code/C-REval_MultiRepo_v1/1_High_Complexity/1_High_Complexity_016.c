/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_016 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 30 */
/* NLOC: 145 */
/* Subsystem: fs */
/* Includes
 * #include "xfs_platform.h"
 * #include "xfs_fs.h"
 * #include "xfs_shared.h"
 * #include "xfs_format.h"
 * #include "xfs_log_format.h"
 * #include "xfs_trans_resv.h"
 * #include "xfs_bit.h"
 * #include "xfs_mount.h"
 * #include "xfs_inode.h"
 * #include "xfs_btree.h"
 * #include "xfs_ialloc.h"
 * #include "xfs_ialloc_btree.h"
 * #include "xfs_alloc.h"
 * #include "xfs_errortag.h"
 * #include "xfs_error.h"
 * #include "xfs_bmap.h"
 * #include "xfs_trans.h"
 * #include "xfs_buf_item.h"
 * #include "xfs_icreate_item.h"
 * #include "xfs_icache.h"
 */
/* Context-Before
 * 		error = xfs_inobt_update(cur, nrec);
 * 		if (error)
 * 			goto error;
 * 	}
 * 
 * 	xfs_btree_del_cursor(cur, XFS_BTREE_NOERROR);
 * 	return 0;
 * error:
 * 	xfs_btree_del_cursor(cur, XFS_BTREE_ERROR);
 * 	return error;
 * }
 * 
 * 
 * /*
 *  * Allocate new inodes in the allocation group specified by agbp.  Returns 0 if
 *  * inodes were allocated in this AG; -EAGAIN if there was no space in this AG so
 *  * the caller knows it can try another AG, a hard -ENOSPC when over the maximum
 *  * inode count threshold, or the usual negative error code for other errors.
 *  */
 * STATIC int
 */
xfs_ialloc_ag_alloc(
	struct xfs_perag	*pag,
	struct xfs_trans	*tp,
	struct xfs_buf		*agbp)
{
	struct xfs_agi		*agi;
	struct xfs_alloc_arg	args;
	int			error;
	xfs_agino_t		newino;		/* new first inode's number */
	xfs_agino_t		newlen;		/* new number of inodes */
	int			isaligned = 0;	/* inode allocation at stripe */
						/* unit boundary */
	/* init. to full chunk */
	struct xfs_inobt_rec_incore rec;
	struct xfs_ino_geometry	*igeo = M_IGEO(tp->t_mountp);
	uint16_t		allocmask = (uint16_t) -1;
	int			do_sparse = 0;

	memset(&args, 0, sizeof(args));
	args.tp = tp;
	args.mp = tp->t_mountp;
	args.fsbno = NULLFSBLOCK;
	args.oinfo = XFS_RMAP_OINFO_INODES;
	args.pag = pag;

#ifdef DEBUG
	/* randomly do sparse inode allocations */
	if (xfs_has_sparseinodes(tp->t_mountp) &&
	    igeo->ialloc_min_blks < igeo->ialloc_blks)
		do_sparse = get_random_u32_below(2);
#endif

	/*
	 * Locking will ensure that we don't have two callers in here
	 * at one time.
	 */
	newlen = igeo->ialloc_inos;
	if (igeo->maxicount &&
	    percpu_counter_read_positive(&args.mp->m_icount) + newlen >
							igeo->maxicount)
		return -ENOSPC;
	args.minlen = args.maxlen = igeo->ialloc_blks;
	/*
	 * First try to allocate inodes contiguous with the last-allocated
	 * chunk of inodes.  If the filesystem is striped, this will fill
	 * an entire stripe unit with inodes.
	 */
	agi = agbp->b_addr;
	newino = be32_to_cpu(agi->agi_newino);
	args.agbno = XFS_AGINO_TO_AGBNO(args.mp, newino) +
		     igeo->ialloc_blks;
	if (do_sparse)
		goto sparse_alloc;
	if (likely(newino != NULLAGINO &&
		  (args.agbno < be32_to_cpu(agi->agi_length)))) {
		args.prod = 1;

		/*
		 * We need to take into account alignment here to ensure that
		 * we don't modify the free list if we fail to have an exact
		 * block. If we don't have an exact match, and every oher
		 * attempt allocation attempt fails, we'll end up cancelling
		 * a dirty transaction and shutting down.
		 *
		 * For an exact allocation, alignment must be 1,
		 * however we need to take cluster alignment into account when
		 * fixing up the freelist. Use the minalignslop field to
		 * indicate that extra blocks might be required for alignment,
		 * but not to use them in the actual exact allocation.
		 */
		args.alignment = 1;
		args.minalignslop = igeo->cluster_align - 1;

		/* Allow space for the inode btree to split. */
		args.minleft = igeo->inobt_maxlevels;
		error = xfs_alloc_vextent_exact_bno(&args,
				xfs_agbno_to_fsb(pag, args.agbno));
		if (error)
			return error;

		/*
		 * This request might have dirtied the transaction if the AG can
		 * satisfy the request, but the exact block was not available.
		 * If the allocation did fail, subsequent requests will relax
		 * the exact agbno requirement and increase the alignment
		 * instead. It is critical that the total size of the request
		 * (len + alignment + slop) does not increase from this point
		 * on, so reset minalignslop to ensure it is not included in
		 * subsequent requests.
		 */
		args.minalignslop = 0;
	}

	if (unlikely(args.fsbno == NULLFSBLOCK)) {
		/*
		 * Set the alignment for the allocation.
		 * If stripe alignment is turned on then align at stripe unit
		 * boundary.
		 * If the cluster size is smaller than a filesystem block
		 * then we're doing I/O for inodes in filesystem block size
		 * pieces, so don't need alignment anyway.
		 */
		isaligned = 0;
		if (igeo->ialloc_align) {
			ASSERT(!xfs_has_noalign(args.mp));
			args.alignment = args.mp->m_dalign;
			isaligned = 1;
		} else
			args.alignment = igeo->cluster_align;
		/*
		 * Allocate a fixed-size extent of inodes.
		 */
		args.prod = 1;
		/*
		 * Allow space for the inode btree to split.
		 */
		args.minleft = igeo->inobt_maxlevels;
		error = xfs_alloc_vextent_near_bno(&args,
				xfs_agbno_to_fsb(pag,
					be32_to_cpu(agi->agi_root)));
		if (error)
			return error;
	}

	/*
	 * If stripe alignment is turned on, then try again with cluster
	 * alignment.
	 */
	if (isaligned && args.fsbno == NULLFSBLOCK) {
		args.alignment = igeo->cluster_align;
		error = xfs_alloc_vextent_near_bno(&args,
				xfs_agbno_to_fsb(pag,
					be32_to_cpu(agi->agi_root)));
		if (error)
			return error;
	}

	/*
	 * Finally, try a sparse allocation if the filesystem supports it and
	 * the sparse allocation length is smaller than a full chunk.
	 */
	if (xfs_has_sparseinodes(args.mp) &&
	    igeo->ialloc_min_blks < igeo->ialloc_blks &&
	    args.fsbno == NULLFSBLOCK) {
sparse_alloc:
		args.alignment = args.mp->m_sb.sb_spino_align;
		args.prod = 1;

		args.minlen = igeo->ialloc_min_blks;
		args.maxlen = args.minlen;

		/*
		 * The inode record will be aligned to full chunk size. We must
		 * prevent sparse allocation from AG boundaries that result in
		 * invalid inode records, such as records that start at agbno 0
		 * or extend beyond the AG.
		 *
		 * Set min agbno to the first chunk aligned, non-zero agbno and
		 * max to one less than the last chunk aligned agbno from the
		 * end of the AG. We subtract 1 from max so that the cluster
		 * allocation alignment takes over and allows allocation within
		 * the last full inode chunk in the AG.
		 */
		args.min_agbno = args.mp->m_sb.sb_inoalignmt;
		args.max_agbno = round_down(xfs_ag_block_count(args.mp,
							pag_agno(pag)),
					    args.mp->m_sb.sb_inoalignmt) - 1;

		error = xfs_alloc_vextent_near_bno(&args,
				xfs_agbno_to_fsb(pag,
					be32_to_cpu(agi->agi_root)));
		if (error)
			return error;

		newlen = XFS_AGB_TO_AGINO(args.mp, args.len);
		ASSERT(newlen <= XFS_INODES_PER_CHUNK);
		allocmask = (1 << (newlen / XFS_INODES_PER_HOLEMASK_BIT)) - 1;
	}

	if (args.fsbno == NULLFSBLOCK)
		return -EAGAIN;

	ASSERT(args.len == args.minlen);

	/*
	 * Stamp and write the inode buffers.
	 *
	 * Seed the new inode cluster with a random generation number. This
	 * prevents short-term reuse of generation numbers if a chunk is
	 * freed and then immediately reallocated. We use random numbers
	 * rather than a linear progression to prevent the next generation
	 * number from being easily guessable.
	 */
	error = xfs_ialloc_inode_init(args.mp, tp, NULL, newlen, pag_agno(pag),
			args.agbno, args.len, get_random_u32());

	if (error)
		return error;
	/*
	 * Convert the results.
	 */
	newino = XFS_AGB_TO_AGINO(args.mp, args.agbno);

	if (xfs_inobt_issparse(~allocmask)) {
		/*
		 * We've allocated a sparse chunk. Align the startino and mask.
		 */
		xfs_align_sparse_ino(args.mp, &newino, &allocmask);

		rec.ir_startino = newino;
		rec.ir_holemask = ~allocmask;
		rec.ir_count = newlen;
		rec.ir_freecount = newlen;
		rec.ir_free = XFS_INOBT_ALL_FREE;

		/*
		 * Insert the sparse record into the inobt and allow for a merge
		 * if necessary. If a merge does occur, rec is updated to the
		 * merged record.
		 */
		error = xfs_inobt_insert_sprec(pag, tp, agbp, &rec);
		if (error == -EFSCORRUPTED) {
			xfs_alert(args.mp,
	"invalid sparse inode record: ino 0x%llx holemask 0x%x count %u",
				  xfs_agino_to_ino(pag, rec.ir_startino),
				  rec.ir_holemask, rec.ir_count);
			xfs_force_shutdown(args.mp, SHUTDOWN_CORRUPT_INCORE);
		}
		if (error)
			return error;

		/*
		 * We can't merge the part we've just allocated as for the inobt
		 * due to finobt semantics. The original record may or may not
		 * exist independent of whether physical inodes exist in this
		 * sparse chunk.
		 *
		 * We must update the finobt record based on the inobt record.
		 * rec contains the fully merged and up to date inobt record
		 * from the previous call. Set merge false to replace any
		 * existing record with this one.
		 */
		if (xfs_has_finobt(args.mp)) {
			error = xfs_finobt_insert_sprec(pag, tp, agbp, &rec);
			if (error)
				return error;
		}
	} else {
		/* full chunk - insert new records to both btrees */
		error = xfs_inobt_insert(pag, tp, agbp, newino, newlen, false);
		if (error)
			return error;

		if (xfs_has_finobt(args.mp)) {
			error = xfs_inobt_insert(pag, tp, agbp, newino,
						 newlen, true);
			if (error)
				return error;
		}
	}

	/*
	 * Update AGI counts and newino.
	 */
	be32_add_cpu(&agi->agi_count, newlen);
	be32_add_cpu(&agi->agi_freecount, newlen);
	pag->pagi_freecount += newlen;
	pag->pagi_count += newlen;
	agi->agi_newino = cpu_to_be32(newino);

	/*
	 * Log allocation group header fields
	 */
	xfs_ialloc_log_agi(tp, agbp,
		XFS_AGI_COUNT | XFS_AGI_FREECOUNT | XFS_AGI_NEWINO);
	/*
	 * Modify/log superblock values for inode count and inode free count.
	 */
	xfs_trans_mod_sb(tp, XFS_TRANS_SB_ICOUNT, (long)newlen);
	xfs_trans_mod_sb(tp, XFS_TRANS_SB_IFREE, (long)newlen);
	return 0;
}
/* Context-After
 * /*
 *  * Try to retrieve the next record to the left/right from the current one.
 *  */
 * STATIC int
 * xfs_ialloc_next_rec(
 * 	struct xfs_btree_cur	*cur,
 * 	xfs_inobt_rec_incore_t	*rec,
 * 	int			*done,
 * 	int			left)
 * {
 * 	int                     error;
 * 	int			i;
 * 
 * 	if (left)
 * 		error = xfs_btree_decrement(cur, 0, &i);
 * 	else
 * 		error = xfs_btree_increment(cur, 0, &i);
 * 
 * 	if (error)
 */
