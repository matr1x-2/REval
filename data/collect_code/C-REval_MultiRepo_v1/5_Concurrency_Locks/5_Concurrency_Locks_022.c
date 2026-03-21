/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_022 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 18 */
/* NLOC: 123 */
/* Subsystem: fs */
/* Includes
 * #include <linux/fs.h>
 * #include <linux/stat.h>
 * #include <linux/time.h>
 * #include <linux/string.h>
 * #include <linux/buffer_head.h>
 * #include <linux/capability.h>
 * #include <linux/bitops.h>
 * #include <linux/bio.h>
 * #include <asm/byteorder.h>
 * #include "ufs_fs.h"
 * #include "ufs.h"
 * #include "swab.h"
 * #include "util.h"
 */
/* Context-Before
 * static void ufs_clear_frags(struct inode *inode, sector_t beg, unsigned int n,
 * 			    int sync)
 * {
 * 	struct buffer_head *bh;
 * 	sector_t end = beg + n;
 * 
 * 	for (; beg < end; ++beg) {
 * 		bh = sb_getblk(inode->i_sb, beg);
 * 		lock_buffer(bh);
 * 		memset(bh->b_data, 0, inode->i_sb->s_blocksize);
 * 		set_buffer_uptodate(bh);
 * 		mark_buffer_dirty(bh);
 * 		unlock_buffer(bh);
 * 		if (IS_SYNC(inode) || sync)
 * 			sync_dirty_buffer(bh);
 * 		brelse(bh);
 * 	}
 * }
 */
u64 ufs_new_fragments(struct inode *inode, void *p, u64 fragment,
			   u64 goal, unsigned count, int *err,
			   struct folio *locked_folio)
{
	struct super_block * sb;
	struct ufs_sb_private_info * uspi;
	struct ufs_super_block_first * usb1;
	unsigned cgno, oldcount, newcount;
	u64 tmp, request, result;
	
	UFSD("ENTER, ino %lu, fragment %llu, goal %llu, count %u\n",
	     inode->i_ino, (unsigned long long)fragment,
	     (unsigned long long)goal, count);
	
	sb = inode->i_sb;
	uspi = UFS_SB(sb)->s_uspi;
	usb1 = ubh_get_usb_first(uspi);
	*err = -ENOSPC;

	mutex_lock(&UFS_SB(sb)->s_lock);
	tmp = ufs_data_ptr_to_cpu(sb, p);

	if (count + ufs_fragnum(fragment) > uspi->s_fpb) {
		ufs_warning(sb, "ufs_new_fragments", "internal warning"
			    " fragment %llu, count %u",
			    (unsigned long long)fragment, count);
		count = uspi->s_fpb - ufs_fragnum(fragment); 
	}
	oldcount = ufs_fragnum (fragment);
	newcount = oldcount + count;

	/*
	 * Somebody else has just allocated our fragments
	 */
	if (oldcount) {
		if (!tmp) {
			ufs_error(sb, "ufs_new_fragments", "internal error, "
				  "fragment %llu, tmp %llu\n",
				  (unsigned long long)fragment,
				  (unsigned long long)tmp);
			mutex_unlock(&UFS_SB(sb)->s_lock);
			return INVBLOCK;
		}
		if (fragment < UFS_I(inode)->i_lastfrag) {
			UFSD("EXIT (ALREADY ALLOCATED)\n");
			mutex_unlock(&UFS_SB(sb)->s_lock);
			return 0;
		}
	}
	else {
		if (tmp) {
			UFSD("EXIT (ALREADY ALLOCATED)\n");
			mutex_unlock(&UFS_SB(sb)->s_lock);
			return 0;
		}
	}

	/*
	 * There is not enough space for user on the device
	 */
	if (unlikely(ufs_freefrags(uspi) <= uspi->s_root_blocks)) {
		if (!capable(CAP_SYS_RESOURCE)) {
			mutex_unlock(&UFS_SB(sb)->s_lock);
			UFSD("EXIT (FAILED)\n");
			return 0;
		}
	}

	if (goal >= uspi->s_size) 
		goal = 0;
	if (goal == 0) 
		cgno = ufs_inotocg (inode->i_ino);
	else
		cgno = ufs_dtog(uspi, goal);
	 
	/*
	 * allocate new fragment
	 */
	if (oldcount == 0) {
		result = ufs_alloc_fragments (inode, cgno, goal, count, err);
		if (result) {
			ufs_clear_frags(inode, result + oldcount,
					newcount - oldcount, locked_folio != NULL);
			*err = 0;
			write_seqlock(&UFS_I(inode)->meta_lock);
			ufs_cpu_to_data_ptr(sb, p, result);
			UFS_I(inode)->i_lastfrag =
				max(UFS_I(inode)->i_lastfrag, fragment + count);
			write_sequnlock(&UFS_I(inode)->meta_lock);
		}
		mutex_unlock(&UFS_SB(sb)->s_lock);
		UFSD("EXIT, result %llu\n", (unsigned long long)result);
		return result;
	}

	/*
	 * resize block
	 */
	result = ufs_add_fragments(inode, tmp, oldcount, newcount);
	if (result) {
		*err = 0;
		read_seqlock_excl(&UFS_I(inode)->meta_lock);
		UFS_I(inode)->i_lastfrag = max(UFS_I(inode)->i_lastfrag,
						fragment + count);
		read_sequnlock_excl(&UFS_I(inode)->meta_lock);
		ufs_clear_frags(inode, result + oldcount, newcount - oldcount,
				locked_folio != NULL);
		mutex_unlock(&UFS_SB(sb)->s_lock);
		UFSD("EXIT, result %llu\n", (unsigned long long)result);
		return result;
	}

	/*
	 * allocate new block and move data
	 */
	if (fs32_to_cpu(sb, usb1->fs_optim) == UFS_OPTSPACE) {
		request = newcount;
		if (uspi->cs_total.cs_nffree < uspi->s_space_to_time)
			usb1->fs_optim = cpu_to_fs32(sb, UFS_OPTTIME);
	} else {
		request = uspi->s_fpb;
		if (uspi->cs_total.cs_nffree > uspi->s_time_to_space)
			usb1->fs_optim = cpu_to_fs32(sb, UFS_OPTSPACE);
	}
	result = ufs_alloc_fragments (inode, cgno, goal, request, err);
	if (result) {
		ufs_clear_frags(inode, result + oldcount, newcount - oldcount,
				locked_folio != NULL);
		mutex_unlock(&UFS_SB(sb)->s_lock);
		ufs_change_blocknr(inode, fragment - oldcount, oldcount,
				   uspi->s_sbbase + tmp,
				   uspi->s_sbbase + result, locked_folio);
		*err = 0;
		write_seqlock(&UFS_I(inode)->meta_lock);
		ufs_cpu_to_data_ptr(sb, p, result);
		UFS_I(inode)->i_lastfrag = max(UFS_I(inode)->i_lastfrag,
						fragment + count);
		write_sequnlock(&UFS_I(inode)->meta_lock);
		if (newcount < request)
			ufs_free_fragments (inode, result + newcount, request - newcount);
		ufs_free_fragments (inode, tmp, oldcount);
		UFSD("EXIT, result %llu\n", (unsigned long long)result);
		return result;
	}

	mutex_unlock(&UFS_SB(sb)->s_lock);
	UFSD("EXIT (FAILED)\n");
	return 0;
}		
/* Context-After
 * static bool try_add_frags(struct inode *inode, unsigned frags)
 * {
 * 	unsigned size = frags * i_blocksize(inode);
 * 	spin_lock(&inode->i_lock);
 * 	__inode_add_bytes(inode, size);
 * 	if (unlikely((u32)inode->i_blocks != inode->i_blocks)) {
 * 		__inode_sub_bytes(inode, size);
 * 		spin_unlock(&inode->i_lock);
 * 		return false;
 * 	}
 * 	spin_unlock(&inode->i_lock);
 * 	return true;
 * }
 * 
 * static u64 ufs_add_fragments(struct inode *inode, u64 fragment,
 * 			     unsigned oldcount, unsigned newcount)
 * {
 * 	struct super_block * sb;
 * 	struct ufs_sb_private_info * uspi;
 */
