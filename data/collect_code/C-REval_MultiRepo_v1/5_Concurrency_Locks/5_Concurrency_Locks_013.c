/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_013 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 64 */
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
 * 	if ((UFS_SB(sb)->s_flags & UFS_CG_MASK) == UFS_CG_44BSD)
 * 		ufs_clusteracct(sb, ucpi, fragment, delta);
 * 
 * 	fs32_add(sb, &ucg->cg_cs.cs_nbfree, delta);
 * 	uspi->cs_total.cs_nbfree += delta;
 * 	fs32_add(sb, &UFS_SB(sb)->fs_cs(ucpi->c_cgx).cs_nbfree, delta);
 * 
 * 	if (uspi->fs_magic != UFS2_MAGIC) {
 * 		unsigned cylno = ufs_cbtocylno(fragment);
 * 
 * 		fs16_add(sb, &ubh_cg_blks(ucpi, cylno,
 * 					  ufs_cbtorpos(fragment)), delta);
 * 		fs32_add(sb, &ubh_cg_blktot(ucpi, cylno), delta);
 * 	}
 * }
 * 
 * /*
 *  * Free 'count' fragments from fragment number 'fragment'
 *  */
 */
void ufs_free_fragments(struct inode *inode, u64 fragment, unsigned count)
{
	struct super_block * sb;
	struct ufs_sb_private_info * uspi;
	struct ufs_cg_private_info * ucpi;
	struct ufs_cylinder_group * ucg;
	unsigned cgno, bit, end_bit, bbase, blkmap, i;
	
	sb = inode->i_sb;
	uspi = UFS_SB(sb)->s_uspi;
	
	UFSD("ENTER, fragment %llu, count %u\n",
	     (unsigned long long)fragment, count);
	
	if (ufs_fragnum(fragment) + count > uspi->s_fpb)
		ufs_error (sb, "ufs_free_fragments", "internal error");

	mutex_lock(&UFS_SB(sb)->s_lock);
	
	cgno = ufs_dtog(uspi, fragment);
	bit = ufs_dtogd(uspi, fragment);
	if (cgno >= uspi->s_ncg) {
		ufs_panic (sb, "ufs_free_fragments", "freeing blocks are outside device");
		goto failed;
	}
		
	ucpi = ufs_load_cylinder (sb, cgno);
	if (!ucpi) 
		goto failed;
	ucg = ubh_get_ucg (UCPI_UBH(ucpi));
	if (!ufs_cg_chkmagic(sb, ucg)) {
		ufs_panic (sb, "ufs_free_fragments", "internal error, bad magic number on cg %u", cgno);
		goto failed;
	}

	end_bit = bit + count;
	bbase = ufs_blknum (bit);
	blkmap = ubh_blkmap (UCPI_UBH(ucpi), ucpi->c_freeoff, bbase);
	ufs_fragacct (sb, blkmap, ucg->cg_frsum, -1);
	for (i = bit; i < end_bit; i++) {
		if (ubh_isclr (UCPI_UBH(ucpi), ucpi->c_freeoff, i))
			ubh_setbit (UCPI_UBH(ucpi), ucpi->c_freeoff, i);
		else 
			ufs_error (sb, "ufs_free_fragments",
				   "bit already cleared for fragment %u", i);
	}

	inode_sub_bytes(inode, count << uspi->s_fshift);
	fs32_add(sb, &ucg->cg_cs.cs_nffree, count);
	uspi->cs_total.cs_nffree += count;
	fs32_add(sb, &UFS_SB(sb)->fs_cs(cgno).cs_nffree, count);
	blkmap = ubh_blkmap (UCPI_UBH(ucpi), ucpi->c_freeoff, bbase);
	ufs_fragacct(sb, blkmap, ucg->cg_frsum, 1);

	/*
	 * Trying to reassemble free fragments into block
	 */
	if (ubh_isblockset(uspi, ucpi, bbase)) {
		fs32_sub(sb, &ucg->cg_cs.cs_nffree, uspi->s_fpb);
		uspi->cs_total.cs_nffree -= uspi->s_fpb;
		fs32_sub(sb, &UFS_SB(sb)->fs_cs(cgno).cs_nffree, uspi->s_fpb);
		adjust_free_blocks(sb, ucg, ucpi, bbase, 1);
	}
	
	ubh_mark_buffer_dirty (USPI_UBH(uspi));
	ubh_mark_buffer_dirty (UCPI_UBH(ucpi));
	if (sb->s_flags & SB_SYNCHRONOUS)
		ubh_sync_block(UCPI_UBH(ucpi));
	ufs_mark_sb_dirty(sb);

	mutex_unlock(&UFS_SB(sb)->s_lock);
	UFSD("EXIT\n");
	return;

failed:
	mutex_unlock(&UFS_SB(sb)->s_lock);
	UFSD("EXIT (FAILED)\n");
	return;
}
/* Context-After
 * /*
 *  * Free 'count' fragments from fragment number 'fragment' (free whole blocks)
 *  */
 * void ufs_free_blocks(struct inode *inode, u64 fragment, unsigned count)
 * {
 * 	struct super_block * sb;
 * 	struct ufs_sb_private_info * uspi;
 * 	struct ufs_cg_private_info * ucpi;
 * 	struct ufs_cylinder_group * ucg;
 * 	unsigned overflow, cgno, bit, end_bit, i;
 * 	
 * 	sb = inode->i_sb;
 * 	uspi = UFS_SB(sb)->s_uspi;
 * 
 * 	UFSD("ENTER, fragment %llu, count %u\n",
 * 	     (unsigned long long)fragment, count);
 * 	
 * 	if ((fragment & uspi->s_fpbmask) || (count & uspi->s_fpbmask)) {
 * 		ufs_error (sb, "ufs_free_blocks", "internal error, "
 */
