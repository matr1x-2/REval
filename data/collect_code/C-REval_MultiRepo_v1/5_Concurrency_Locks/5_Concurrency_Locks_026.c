/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_026 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 32 */
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
 * 		const char *procname = entry->procname;
 * 		link = find_entry(&tmp_head, dir, procname, strlen(procname));
 * 		if (!link)
 * 			return false;
 * 		if (S_ISDIR(link->mode) && S_ISDIR(entry->mode))
 * 			continue;
 * 		if (S_ISLNK(link->mode) && (link->data == link_root))
 * 			continue;
 * 		return false;
 * 	}
 * 
 * 	/* The checks passed.  Increase the registration count on the links */
 * 	list_for_each_table_entry(entry, header) {
 * 		const char *procname = entry->procname;
 * 		link = find_entry(&tmp_head, dir, procname, strlen(procname));
 * 		tmp_head->nreg++;
 * 	}
 * 	return true;
 * }
 */
static int insert_links(struct ctl_table_header *head)
{
	struct ctl_table_set *root_set = &sysctl_table_root.default_set;
	struct ctl_dir *core_parent;
	struct ctl_table_header *links;
	int err;

	if (head->set == root_set)
		return 0;

	core_parent = xlate_dir(root_set, head->parent);
	if (IS_ERR(core_parent))
		return 0;

	if (get_links(core_parent, head, head->root))
		return 0;

	core_parent->header.nreg++;
	spin_unlock(&sysctl_lock);

	links = new_links(core_parent, head);

	spin_lock(&sysctl_lock);
	err = -ENOMEM;
	if (!links)
		goto out;

	err = 0;
	if (get_links(core_parent, head, head->root)) {
		kfree(links);
		goto out;
	}

	err = insert_header(core_parent, links);
	if (err)
		kfree(links);
out:
	drop_sysctl_table(&core_parent->header);
	return err;
}
/* Context-After
 * /* Find the directory for the ctl_table. If one is not found create it. */
 * static struct ctl_dir *sysctl_mkdir_p(struct ctl_dir *dir, const char *path)
 * {
 * 	const char *name, *nextname;
 * 
 * 	for (name = path; name; name = nextname) {
 * 		int namelen;
 * 		nextname = strchr(name, '/');
 * 		if (nextname) {
 * 			namelen = nextname - name;
 * 			nextname++;
 * 		} else {
 * 			namelen = strlen(name);
 * 		}
 * 		if (namelen == 0)
 * 			continue;
 * 
 * 		/*
 * 		 * namelen ensures if name is "foo/bar/yay" only foo is
 */
