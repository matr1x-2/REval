/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_033 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 34 */
/* Subsystem: fs */
/* Includes
 * #include <linux/module.h>
 * #include <linux/fs.h>
 * #include <linux/slab.h>
 * #include <linux/file.h>
 * #include <linux/fileattr.h>
 * #include <linux/splice.h>
 * #include <linux/xattr.h>
 * #include <linux/security.h>
 * #include <linux/uaccess.h>
 * #include <linux/sched/signal.h>
 * #include <linux/cred.h>
 * #include <linux/namei.h>
 * #include <linux/ratelimit.h>
 * #include <linux/exportfs.h>
 * #include "overlayfs.h"
 */
/* Context-Before
 * }
 * 
 * struct ovl_copy_up_ctx {
 * 	struct dentry *parent;
 * 	struct dentry *dentry;
 * 	struct path lowerpath;
 * 	struct kstat stat;
 * 	struct kstat pstat;
 * 	const char *link;
 * 	struct dentry *destdir;
 * 	struct qstr destname;
 * 	struct dentry *workdir;
 * 	const struct ovl_fh *origin_fh;
 * 	bool origin;
 * 	bool indexed;
 * 	bool metacopy;
 * 	bool metacopy_digest;
 * 	bool metadata_fsync;
 * };
 */
static int ovl_link_up(struct ovl_copy_up_ctx *c)
{
	int err;
	struct dentry *upper;
	struct dentry *upperdir = ovl_dentry_upper(c->parent);
	struct ovl_fs *ofs = OVL_FS(c->dentry->d_sb);
	struct inode *udir = d_inode(upperdir);

	ovl_start_write(c->dentry);

	/* Mark parent "impure" because it may now contain non-pure upper */
	err = ovl_set_impure(c->parent, upperdir);
	if (err)
		goto out;

	err = ovl_set_nlink_lower(c->dentry);
	if (err)
		goto out;

	upper = ovl_start_creating_upper(ofs, upperdir,
					 &QSTR_LEN(c->dentry->d_name.name,
						   c->dentry->d_name.len));
	err = PTR_ERR(upper);
	if (!IS_ERR(upper)) {
		err = ovl_do_link(ofs, ovl_dentry_upper(c->dentry), udir, upper);

		if (!err) {
			/* Restore timestamps on parent (best effort) */
			ovl_set_timestamps(ofs, upperdir, &c->pstat);
			ovl_dentry_set_upper_alias(c->dentry);
			ovl_dentry_update_reval(c->dentry, upper);
		}
		end_creating(upper);
	}
	if (err)
		goto out;

	err = ovl_set_nlink_upper(c->dentry);

out:
	ovl_end_write(c->dentry);
	return err;
}
/* Context-After
 * static int ovl_copy_up_data(struct ovl_copy_up_ctx *c, const struct path *temp)
 * {
 * 	struct ovl_fs *ofs = OVL_FS(c->dentry->d_sb);
 * 	struct file *new_file;
 * 	int err;
 * 
 * 	if (!S_ISREG(c->stat.mode) || c->metacopy || !c->stat.size)
 * 		return 0;
 * 
 * 	new_file = ovl_path_open(temp, O_LARGEFILE | O_WRONLY);
 * 	if (IS_ERR(new_file))
 * 		return PTR_ERR(new_file);
 * 
 * 	err = ovl_copy_up_file(ofs, c->dentry, new_file, c->stat.size,
 * 			       !c->metadata_fsync);
 * 	fput(new_file);
 * 
 * 	return err;
 * }
 */
