/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_018 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 43 */
/* Subsystem: fs */
/* Includes
 * #include <linux/init.h>
 * #include <linux/sysctl.h>
 * #include <linux/poll.h>
 * #include <linux/proc_fs.h>
 * #include <linux/printk.h>
 * #include <linux/security.h>
 * #include <linux/sched.h>
 * #include <linux/cred.h>
 * #include <linux/namei.h>
 * #include <linux/mm.h>
 * #include <linux/uio.h>
 * #include <linux/module.h>
 * #include <linux/bpf-cgroup.h>
 * #include <linux/mount.h>
 * #include <linux/kmemleak.h>
 * #include <linux/lockdep.h>
 * #include "internal.h"
 */
/* Context-Before
 * 	memcpy(new_name, name, namelen);
 * 	table[0].procname = new_name;
 * 	table[0].mode = S_IFDIR|S_IRUGO|S_IXUGO;
 * 	init_header(&new->header, set->dir.header.root, set, node, table, 1);
 * 
 * 	return new;
 * }
 * 
 * /**
 *  * get_subdir - find or create a subdir with the specified name.
 *  * @dir:  Directory to create the subdirectory in
 *  * @name: The name of the subdirectory to find or create
 *  * @namelen: The length of name
 *  *
 *  * Takes a directory with an elevated reference count so we know that
 *  * if we drop the lock the directory will not go away.  Upon success
 *  * the reference is moved from @dir to the returned subdirectory.
 *  * Upon error an error code is returned and the reference on @dir is
 *  * simply dropped.
 *  */
 */
static struct ctl_dir *get_subdir(struct ctl_dir *dir,
				  const char *name, int namelen)
{
	struct ctl_table_set *set = dir->header.set;
	struct ctl_dir *subdir, *new = NULL;
	int err;

	spin_lock(&sysctl_lock);
	subdir = find_subdir(dir, name, namelen);
	if (!IS_ERR(subdir))
		goto found;
	if (PTR_ERR(subdir) != -ENOENT)
		goto failed;

	spin_unlock(&sysctl_lock);
	new = new_dir(set, name, namelen);
	spin_lock(&sysctl_lock);
	subdir = ERR_PTR(-ENOMEM);
	if (!new)
		goto failed;

	/* Was the subdir added while we dropped the lock? */
	subdir = find_subdir(dir, name, namelen);
	if (!IS_ERR(subdir))
		goto found;
	if (PTR_ERR(subdir) != -ENOENT)
		goto failed;

	/* Nope.  Use the our freshly made directory entry. */
	err = insert_header(dir, &new->header);
	subdir = ERR_PTR(err);
	if (err)
		goto failed;
	subdir = new;
found:
	subdir->header.nreg++;
failed:
	if (IS_ERR(subdir)) {
		pr_err("sysctl could not get directory: ");
		sysctl_print_dir(dir);
		pr_cont("%*.*s %ld\n", namelen, namelen, name,
			PTR_ERR(subdir));
	}
	drop_sysctl_table(&dir->header);
	if (new)
		drop_sysctl_table(&new->header);
	spin_unlock(&sysctl_lock);
	return subdir;
}
/* Context-After
 * static struct ctl_dir *xlate_dir(struct ctl_table_set *set, struct ctl_dir *dir)
 * {
 * 	struct ctl_dir *parent;
 * 	const char *procname;
 * 	if (!dir->header.parent)
 * 		return &set->dir;
 * 	parent = xlate_dir(set, dir->header.parent);
 * 	if (IS_ERR(parent))
 * 		return parent;
 * 	procname = dir->header.ctl_table[0].procname;
 * 	return find_subdir(parent, procname, strlen(procname));
 * }
 * 
 * static int sysctl_follow_link(struct ctl_table_header **phead,
 * 	const struct ctl_table **pentry)
 * {
 * 	struct ctl_table_header *head;
 * 	const struct ctl_table *entry;
 * 	struct ctl_table_root *root;
 */
