/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_025 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 41 */
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
 * 		nilfs_sufile_mod_counter(header_bh, ncleaned, 0);
 * 		nilfs_mdt_mark_dirty(sufile);
 * 	}
 * 	brelse(header_bh);
 * out:
 * 	return ret;
 * }
 * 
 * /**
 *  * nilfs_sufile_resize - resize segment array
 *  * @sufile: inode of segment usage file
 *  * @newnsegs: new number of segments
 *  *
 *  * Return: 0 on success, or one of the following negative error codes on
 *  * failure:
 *  * * %-EBUSY	- Dirty or active segments exist in the region to be truncated.
 *  * * %-EIO	- I/O error (including metadata corruption).
 *  * * %-ENOMEM	- Insufficient memory available.
 *  * * %-ENOSPC	- Enough free space is not left for shrinking.
 *  */
 */
int nilfs_sufile_resize(struct inode *sufile, __u64 newnsegs)
{
	struct the_nilfs *nilfs = sufile->i_sb->s_fs_info;
	struct buffer_head *header_bh;
	struct nilfs_sufile_header *header;
	struct nilfs_sufile_info *sui = NILFS_SUI(sufile);
	unsigned long nsegs, nrsvsegs;
	int ret = 0;

	down_write(&NILFS_MDT(sufile)->mi_sem);

	nsegs = nilfs_sufile_get_nsegments(sufile);
	if (nsegs == newnsegs)
		goto out;

	ret = -ENOSPC;
	nrsvsegs = nilfs_nrsvsegs(nilfs, newnsegs);
	if (newnsegs < nsegs && nsegs - newnsegs + nrsvsegs > sui->ncleansegs)
		goto out;

	ret = nilfs_sufile_get_header_block(sufile, &header_bh);
	if (ret < 0)
		goto out;

	if (newnsegs > nsegs) {
		sui->ncleansegs += newnsegs - nsegs;
	} else /* newnsegs < nsegs */ {
		ret = nilfs_sufile_truncate_range(sufile, newnsegs, nsegs - 1);
		if (ret < 0)
			goto out_header;

		sui->ncleansegs -= nsegs - newnsegs;

		/*
		 * If the sufile is successfully truncated, immediately adjust
		 * the segment allocation space while locking the semaphore
		 * "mi_sem" so that nilfs_sufile_alloc() never allocates
		 * segments in the truncated space.
		 */
		sui->allocmax = newnsegs - 1;
		sui->allocmin = 0;
	}

	header = kmap_local_folio(header_bh->b_folio, 0);
	header->sh_ncleansegs = cpu_to_le64(sui->ncleansegs);
	kunmap_local(header);

	mark_buffer_dirty(header_bh);
	nilfs_mdt_mark_dirty(sufile);
	nilfs_set_nsegments(nilfs, newnsegs);

out_header:
	brelse(header_bh);
out:
	up_write(&NILFS_MDT(sufile)->mi_sem);
	return ret;
}
/* Context-After
 * /**
 *  * nilfs_sufile_get_suinfo - get segment usage information
 *  * @sufile: inode of segment usage file
 *  * @segnum: segment number to start looking
 *  * @buf:    array of suinfo
 *  * @sisz:   byte size of suinfo
 *  * @nsi:    size of suinfo array
 *  *
 *  * Return: Count of segment usage info items stored in the output buffer on
 *  * success, or one of the following negative error codes on failure:
 *  * * %-EIO	- I/O error (including metadata corruption).
 *  * * %-ENOMEM	- Insufficient memory available.
 *  */
 * ssize_t nilfs_sufile_get_suinfo(struct inode *sufile, __u64 segnum, void *buf,
 * 				unsigned int sisz, size_t nsi)
 * {
 * 	struct buffer_head *su_bh;
 * 	struct nilfs_segment_usage *su;
 * 	struct nilfs_suinfo *si = buf;
 */
