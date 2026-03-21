/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_046 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 39 */
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
 * 	res = ovl_do_getxattr(path, name, NULL, 0);
 * 	if (res == -ENODATA || res == -EOPNOTSUPP)
 * 		res = 0;
 * 
 * 	if (res > 0) {
 * 		buf = kzalloc(res, GFP_KERNEL);
 * 		if (!buf)
 * 			return -ENOMEM;
 * 
 * 		res = ovl_do_getxattr(path, name, buf, res);
 * 		if (res < 0)
 * 			kfree(buf);
 * 		else
 * 			*value = buf;
 * 	}
 * 	return res;
 * }
 * 
 * /* Copy up data of an inode which was copied up metadata only in the past. */
 */
static int ovl_copy_up_meta_inode_data(struct ovl_copy_up_ctx *c)
{
	struct ovl_fs *ofs = OVL_FS(c->dentry->d_sb);
	struct path upperpath;
	int err;
	char *capability = NULL;
	ssize_t cap_size;

	ovl_path_upper(c->dentry, &upperpath);
	if (WARN_ON(upperpath.dentry == NULL))
		return -EIO;

	if (c->stat.size) {
		err = cap_size = ovl_getxattr_value(&upperpath, XATTR_NAME_CAPS,
						    &capability);
		if (cap_size < 0)
			goto out;
	}

	err = ovl_copy_up_data(c, &upperpath);
	if (err)
		goto out_free;

	/*
	 * Writing to upper file will clear security.capability xattr. We
	 * don't want that to happen for normal copy-up operation.
	 */
	ovl_start_write(c->dentry);
	if (capability) {
		err = ovl_do_setxattr(ofs, upperpath.dentry, XATTR_NAME_CAPS,
				      capability, cap_size, 0);
	}
	if (!err) {
		err = ovl_removexattr(ofs, upperpath.dentry,
				      OVL_XATTR_METACOPY);
	}
	ovl_end_write(c->dentry);
	if (err)
		goto out_free;

	ovl_clear_flag(OVL_HAS_DIGEST, d_inode(c->dentry));
	ovl_clear_flag(OVL_VERIFIED_DIGEST, d_inode(c->dentry));
	ovl_set_upperdata(d_inode(c->dentry));
out_free:
	kfree(capability);
out:
	return err;
}
/* Context-After
 * static int ovl_copy_up_one(struct dentry *parent, struct dentry *dentry,
 * 			   int flags)
 * {
 * 	int err;
 * 	DEFINE_DELAYED_CALL(done);
 * 	struct path parentpath;
 * 	struct ovl_copy_up_ctx ctx = {
 * 		.parent = parent,
 * 		.dentry = dentry,
 * 		.workdir = ovl_workdir(dentry),
 * 	};
 * 
 * 	if (WARN_ON(!ctx.workdir))
 * 		return -EROFS;
 * 
 * 	ovl_path_lower(dentry, &ctx.lowerpath);
 * 	err = vfs_getattr(&ctx.lowerpath, &ctx.stat,
 * 			  STATX_BASIC_STATS, AT_STATX_SYNC_AS_STAT);
 * 	if (err)
 */
