/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_028 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 2 */
/* NLOC: 17 */
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
 * 	if (ret < 0) {
 * 		_leave(" = %d [iter]", ret);
 * 		return ret;
 * 	}
 * 
 * 	if (!cookie.found) {
 * 		_leave(" = -ENOENT [not found]");
 * 		return -ENOENT;
 * 	}
 * 
 * 	*fid = cookie.fid;
 * 	_leave(" = 0 { vn=%llu u=%u }", fid->vnode, fid->unique);
 * 	return 0;
 * }
 * 
 * /*
 *  * search the directory for a name
 *  * - if afs_dir_iterate_block() spots this function, it'll pass the FID
 *  *   uniquifier through dtype
 *  */
 */
static bool afs_lookup_filldir(struct dir_context *ctx, const char *name,
			      int nlen, loff_t fpos, u64 ino, unsigned dtype)
{
	struct afs_lookup_cookie *cookie =
		container_of(ctx, struct afs_lookup_cookie, ctx);

	_enter("{%s,%u},%s,%u,,%llu,%u",
	       cookie->name.name, cookie->name.len, name, nlen,
	       (unsigned long long) ino, dtype);

	/* insanity checks first */
	BUILD_BUG_ON(sizeof(union afs_xdr_dir_block) != 2048);
	BUILD_BUG_ON(sizeof(union afs_xdr_dirent) != 32);

	if (cookie->nr_fids < 50) {
		cookie->fids[cookie->nr_fids].vnode	= ino;
		cookie->fids[cookie->nr_fids].unique	= dtype;
		cookie->nr_fids++;
	}

	return cookie->nr_fids < 50;
}
/* Context-After
 * /*
 *  * Deal with the result of a successful lookup operation.  Turn all the files
 *  * into inodes and save the first one - which is the one we actually want.
 *  */
 * static void afs_do_lookup_success(struct afs_operation *op)
 * {
 * 	struct afs_vnode_param *vp;
 * 	struct afs_vnode *vnode;
 * 	struct inode *inode;
 * 	u32 abort_code;
 * 	int i;
 * 
 * 	_enter("");
 * 
 * 	for (i = 0; i < op->nr_files; i++) {
 * 		switch (i) {
 * 		case 0:
 * 			vp = &op->file[0];
 * 			abort_code = vp->scb.status.abort_code;
 */
