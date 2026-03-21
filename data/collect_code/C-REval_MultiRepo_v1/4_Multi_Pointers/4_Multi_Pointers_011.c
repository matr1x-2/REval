/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_011 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 19 */
/* Subsystem: fs */
/* Includes
 * #include <linux/fs.h>
 * #include <linux/types.h>
 * #include <linux/slab.h>
 * #include <linux/string.h>
 * #include <linux/highmem.h>
 * #include <cluster/masklog.h>
 * #include "ocfs2.h"
 * #include "alloc.h"
 * #include "blockcheck.h"
 * #include "dlmglue.h"
 * #include "inode.h"
 * #include "journal.h"
 * #include "localalloc.h"
 * #include "suballoc.h"
 * #include "super.h"
 * #include "sysfile.h"
 * #include "uptodate.h"
 * #include "ocfs2_trace.h"
 * #include "buffer_head_io.h"
 */
/* Context-Before
 * 		if (rc)
 * 			goto free_bh;
 * 	}
 * 
 * 	rc = ocfs2_validate_gd_parent(inode->i_sb, di, tmp, 0);
 * 	if (rc)
 * 		goto free_bh;
 * 
 * 	/* If ocfs2_read_block() got us a new bh, pass it up. */
 * 	if (!*bh)
 * 		*bh = tmp;
 * 
 * 	return rc;
 * 
 * free_bh:
 * 	brelse(tmp);
 * out:
 * 	return rc;
 * }
 */
int ocfs2_read_group_descriptor(struct inode *inode, struct ocfs2_dinode *di,
				u64 gd_blkno, struct buffer_head **bh)
{
	int rc;
	struct buffer_head *tmp = *bh;

	rc = ocfs2_read_block(INODE_CACHE(inode), gd_blkno, &tmp,
			      ocfs2_validate_group_descriptor);
	if (rc)
		goto out;

	rc = ocfs2_validate_gd_parent(inode->i_sb, di, tmp, 0);
	if (rc) {
		brelse(tmp);
		goto out;
	}

	/* If ocfs2_read_block() got us a new bh, pass it up. */
	if (!*bh)
		*bh = tmp;

out:
	return rc;
}
/* Context-After
 * static void ocfs2_bg_discontig_add_extent(struct ocfs2_super *osb,
 * 					  struct ocfs2_group_desc *bg,
 * 					  struct ocfs2_chain_list *cl,
 * 					  u64 p_blkno, unsigned int clusters)
 * {
 * 	struct ocfs2_extent_list *el = &bg->bg_list;
 * 	struct ocfs2_extent_rec *rec;
 * 
 * 	BUG_ON(!ocfs2_supports_discontig_bg(osb));
 * 	if (!el->l_next_free_rec)
 * 		el->l_count = cpu_to_le16(ocfs2_extent_recs_per_gd(osb->sb));
 * 	rec = &el->l_recs[le16_to_cpu(el->l_next_free_rec)];
 * 	rec->e_blkno = cpu_to_le64(p_blkno);
 * 	rec->e_cpos = cpu_to_le32(le16_to_cpu(bg->bg_bits) /
 * 				  le16_to_cpu(cl->cl_bpc));
 * 	rec->e_leaf_clusters = cpu_to_le16(clusters);
 * 	le16_add_cpu(&bg->bg_bits, clusters * le16_to_cpu(cl->cl_bpc));
 * 	le16_add_cpu(&bg->bg_free_bits_count,
 * 		     clusters * le16_to_cpu(cl->cl_bpc));
 */
