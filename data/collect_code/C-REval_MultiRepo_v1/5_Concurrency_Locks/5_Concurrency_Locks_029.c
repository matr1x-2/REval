/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_029 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 15 */
/* Subsystem: fs */
/* Includes
 * #include <linux/pagemap.h>
 * #include <linux/namei.h>
 * #include <linux/backing-dev.h>
 * #include <linux/capability.h>
 * #include <linux/sched.h>
 * #include <linux/lockdep.h>
 * #include <linux/slab.h>
 * #include <linux/configfs.h>
 * #include "configfs_internal.h"
 */
/* Context-Before
 * 	struct configfs_attribute *attr;
 * 
 * 	BUG_ON(!sd || !sd->s_element);
 * 
 * 	/* These always have a dentry, so use that */
 * 	if (sd->s_type & (CONFIGFS_DIR | CONFIGFS_ITEM_LINK))
 * 		return sd->s_dentry->d_name.name;
 * 
 * 	if (sd->s_type & (CONFIGFS_ITEM_ATTR | CONFIGFS_ITEM_BIN_ATTR)) {
 * 		attr = sd->s_element;
 * 		return attr->ca_name;
 * 	}
 * 	return NULL;
 * }
 * 
 * 
 * /*
 *  * Unhashes the dentry corresponding to given configfs_dirent
 *  * Called with parent inode's i_mutex held.
 *  */
 */
void configfs_drop_dentry(struct configfs_dirent * sd, struct dentry * parent)
{
	struct dentry * dentry = sd->s_dentry;

	if (dentry) {
		spin_lock(&dentry->d_lock);
		if (simple_positive(dentry)) {
			dget_dlock(dentry);
			__d_drop(dentry);
			spin_unlock(&dentry->d_lock);
			__simple_unlink(d_inode(parent), dentry);
			dput(dentry);
		} else
			spin_unlock(&dentry->d_lock);
	}
}
/* Context-After: <empty> */
