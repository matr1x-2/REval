/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_023 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 21 */
/* Subsystem: fs */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/fs.h>
 * #include <linux/namei.h>
 * #include <linux/pagemap.h>
 * #include <linux/swap.h>
 * #include <linux/ctype.h>
 * #include <linux/sched.h>
 * #include <linux/iversion.h>
 * #include <linux/iov_iter.h>
 * #include <linux/task_io_accounting_ops.h>
 * #include "internal.h"
 * #include "afs_fs.h"
 * #include "xdr_fs.h"
 */
/* Context-Before
 * 	_leave(" = %d", ret);
 * 	return ret;
 * }
 * 
 * /*
 *  * read an AFS directory
 *  */
 * static int afs_readdir(struct file *file, struct dir_context *ctx)
 * {
 * 	afs_dataversion_t dir_version;
 * 
 * 	return afs_dir_iterate(file_inode(file), ctx, file, &dir_version);
 * }
 * 
 * /*
 *  * Search the directory for a single name
 *  * - if afs_dir_iterate_block() spots this function, it'll pass the FID
 *  *   uniquifier through dtype
 *  */
 */
static bool afs_lookup_one_filldir(struct dir_context *ctx, const char *name,
				  int nlen, loff_t fpos, u64 ino, unsigned dtype)
{
	struct afs_lookup_one_cookie *cookie =
		container_of(ctx, struct afs_lookup_one_cookie, ctx);

	_enter("{%s,%u},%s,%u,,%llu,%u",
	       cookie->name.name, cookie->name.len, name, nlen,
	       (unsigned long long) ino, dtype);

	/* insanity checks first */
	BUILD_BUG_ON(sizeof(union afs_xdr_dir_block) != 2048);
	BUILD_BUG_ON(sizeof(union afs_xdr_dirent) != 32);

	if (cookie->name.len != nlen ||
	    memcmp(cookie->name.name, name, nlen) != 0) {
		_leave(" = true [keep looking]");
		return true;
	}

	cookie->fid.vnode = ino;
	cookie->fid.unique = dtype;
	cookie->found = 1;

	_leave(" = false [found]");
	return false;
}
/* Context-After
 * /*
 *  * Do a lookup of a single name in a directory
 *  * - just returns the FID the dentry name maps to if found
 *  */
 * static int afs_do_lookup_one(struct inode *dir, const struct qstr *name,
 * 			     struct afs_fid *fid,
 * 			     afs_dataversion_t *_dir_version)
 * {
 * 	struct afs_super_info *as = dir->i_sb->s_fs_info;
 * 	struct afs_lookup_one_cookie cookie = {
 * 		.ctx.actor = afs_lookup_one_filldir,
 * 		.name = *name,
 * 		.fid.vid = as->volume->vid
 * 	};
 * 	int ret;
 * 
 * 	_enter("{%lu},{%.*s},", dir->i_ino, name->len, name->name);
 * 
 * 	/* search the directory */
 */
