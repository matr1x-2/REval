/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_040 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 12 */
/* NLOC: 58 */
/* Subsystem: fs */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/fs.h>
 * #include <linux/string.h>
 * #include <linux/buffer_head.h>
 * #include <linux/errno.h>
 * #include "mdt.h"
 * #include "sufile.h"
 * #include <trace/events/nilfs2.h>
 */
/* Context-Before
 *  * @nsegs: size of @segnumv array
 *  * @create: creation flag
 *  * @ndone: place to store number of modified segments on @segnumv
 *  * @dofunc: primitive operation for the update
 *  *
 *  * Description: nilfs_sufile_updatev() repeatedly calls @dofunc
 *  * against the given array of segments.  The @dofunc is called with
 *  * buffers of a header block and the sufile block in which the target
 *  * segment usage entry is contained.  If @ndone is given, the number
 *  * of successfully modified segments from the head is stored in the
 *  * place @ndone points to.
 *  *
 *  * Return: 0 on success, or one of the following negative error codes on
 *  * failure:
 *  * * %-EINVAL	- Invalid segment usage number
 *  * * %-EIO	- I/O error (including metadata corruption).
 *  * * %-ENOENT	- Given segment usage is in hole block (may be returned if
 *  *		  @create is zero)
 *  * * %-ENOMEM	- Insufficient memory available.
 *  */
 */
int nilfs_sufile_updatev(struct inode *sufile, __u64 *segnumv, size_t nsegs,
			 int create, size_t *ndone,
			 void (*dofunc)(struct inode *, __u64,
					struct buffer_head *,
					struct buffer_head *))
{
	struct buffer_head *header_bh, *bh;
	unsigned long blkoff, prev_blkoff;
	__u64 *seg;
	size_t nerr = 0, n = 0;
	int ret = 0;

	if (unlikely(nsegs == 0))
		goto out;

	down_write(&NILFS_MDT(sufile)->mi_sem);
	for (seg = segnumv; seg < segnumv + nsegs; seg++) {
		if (unlikely(*seg >= nilfs_sufile_get_nsegments(sufile))) {
			nilfs_warn(sufile->i_sb,
				   "%s: invalid segment number: %llu",
				   __func__, (unsigned long long)*seg);
			nerr++;
		}
	}
	if (nerr > 0) {
		ret = -EINVAL;
		goto out_sem;
	}

	ret = nilfs_sufile_get_header_block(sufile, &header_bh);
	if (ret < 0)
		goto out_sem;

	seg = segnumv;
	blkoff = nilfs_sufile_get_blkoff(sufile, *seg);
	ret = nilfs_mdt_get_block(sufile, blkoff, create, NULL, &bh);
	if (ret < 0)
		goto out_header;

	for (;;) {
		dofunc(sufile, *seg, header_bh, bh);

		if (++seg >= segnumv + nsegs)
			break;
		prev_blkoff = blkoff;
		blkoff = nilfs_sufile_get_blkoff(sufile, *seg);
		if (blkoff == prev_blkoff)
			continue;

		/* get different block */
		brelse(bh);
		ret = nilfs_mdt_get_block(sufile, blkoff, create, NULL, &bh);
		if (unlikely(ret < 0))
			goto out_header;
	}
	brelse(bh);

 out_header:
	n = seg - segnumv;
	brelse(header_bh);
 out_sem:
	up_write(&NILFS_MDT(sufile)->mi_sem);
 out:
	if (ndone)
		*ndone = n;
	return ret;
}
/* Context-After
 * int nilfs_sufile_update(struct inode *sufile, __u64 segnum, int create,
 * 			void (*dofunc)(struct inode *, __u64,
 * 				       struct buffer_head *,
 * 				       struct buffer_head *))
 * {
 * 	struct buffer_head *header_bh, *bh;
 * 	int ret;
 * 
 * 	if (unlikely(segnum >= nilfs_sufile_get_nsegments(sufile))) {
 * 		nilfs_warn(sufile->i_sb, "%s: invalid segment number: %llu",
 * 			   __func__, (unsigned long long)segnum);
 * 		return -EINVAL;
 * 	}
 * 	down_write(&NILFS_MDT(sufile)->mi_sem);
 * 
 * 	ret = nilfs_sufile_get_header_block(sufile, &header_bh);
 * 	if (ret < 0)
 * 		goto out_sem;
 */
