/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_013 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 11 */
/* NLOC: 39 */
/* Subsystem: fs */
/* Includes
 * #include <linux/fs.h>
 * #include <linux/namei.h>
 * #include <linux/xattr.h>
 * #include <linux/security.h>
 * #include <linux/cred.h>
 * #include <linux/module.h>
 * #include <linux/posix_acl.h>
 * #include <linux/posix_acl_xattr.h>
 * #include <linux/atomic.h>
 * #include <linux/ratelimit.h>
 * #include <linux/backing-file.h>
 * #include "overlayfs.h"
 */
/* Context-Before
 * 	inode = d_inode(old);
 * 	ihold(inode);
 * 
 * 	err = ovl_create_or_link(new, inode,
 * 			&(struct ovl_cattr) {.hardlink = ovl_dentry_upper(old)},
 * 			ovl_type_origin(old));
 * 	if (err)
 * 		iput(inode);
 * 
 * out_nlink_end:
 * 	ovl_nlink_end(old);
 * out:
 * 	return err;
 * }
 * 
 * static bool ovl_matches_upper(struct dentry *dentry, struct dentry *upper)
 * {
 * 	return d_inode(ovl_dentry_upper(dentry)) == d_inode(upper);
 * }
 */
static int ovl_remove_and_whiteout(struct dentry *dentry,
				   struct list_head *list)
{
	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
	struct dentry *workdir = ovl_workdir(dentry);
	struct dentry *upperdir = ovl_dentry_upper(dentry->d_parent);
	struct dentry *upper;
	struct dentry *opaquedir = NULL;
	int err;

	if (WARN_ON(!workdir))
		return -EROFS;

	if (!list_empty(list)) {
		opaquedir = ovl_clear_empty(dentry, list);
		err = PTR_ERR(opaquedir);
		if (IS_ERR(opaquedir))
			goto out;
	}

	upper = ovl_lookup_upper_unlocked(ofs, dentry->d_name.name, upperdir,
					  dentry->d_name.len);
	err = PTR_ERR(upper);
	if (IS_ERR(upper))
		goto out_dput;

	err = -ESTALE;
	if ((opaquedir && upper != opaquedir) ||
	    (!opaquedir && ovl_dentry_upper(dentry) &&
	     !ovl_matches_upper(dentry, upper))) {
		goto out_dput_upper;
	}

	err = ovl_cleanup_and_whiteout(ofs, upperdir, upper);
	if (!err)
		ovl_dir_modified(dentry->d_parent, true);

	d_drop(dentry);
out_dput_upper:
	dput(upper);
out_dput:
	dput(opaquedir);
out:
	return err;
}
/* Context-After
 * static int ovl_remove_upper(struct dentry *dentry, bool is_dir,
 * 			    struct list_head *list)
 * {
 * 	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
 * 	struct dentry *upperdir = ovl_dentry_upper(dentry->d_parent);
 * 	struct inode *dir = upperdir->d_inode;
 * 	struct dentry *upper;
 * 	struct dentry *opaquedir = NULL;
 * 	int err;
 * 
 * 	if (!list_empty(list)) {
 * 		opaquedir = ovl_clear_empty(dentry, list);
 * 		err = PTR_ERR(opaquedir);
 * 		if (IS_ERR(opaquedir))
 * 			goto out;
 * 	}
 * 
 * 	upper = ovl_start_removing_upper(ofs, upperdir,
 * 					 &QSTR_LEN(dentry->d_name.name,
 */
