/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_014 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 2 */
/* NLOC: 16 */
/* Subsystem: kernel */
/* Includes
 * #include "cgroup-internal.h"
 * #include <linux/ctype.h>
 * #include <linux/kmod.h>
 * #include <linux/sort.h>
 * #include <linux/delay.h>
 * #include <linux/mm.h>
 * #include <linux/sched/signal.h>
 * #include <linux/sched/task.h>
 * #include <linux/magic.h>
 * #include <linux/slab.h>
 * #include <linux/string.h>
 * #include <linux/vmalloc.h>
 * #include <linux/delayacct.h>
 * #include <linux/pid_namespace.h>
 * #include <linux/cgroupstats.h>
 * #include <linux/fs_parser.h>
 * #include <trace/events/cgroup.h>
 */
/* Context-Before
 * 	struct delayed_work destroy_dwork;
 * };
 * 
 * /*
 *  * Used to destroy all pidlists lingering waiting for destroy timer.  None
 *  * should be left afterwards.
 *  */
 * void cgroup1_pidlist_destroy_all(struct cgroup *cgrp)
 * {
 * 	struct cgroup_pidlist *l, *tmp_l;
 * 
 * 	mutex_lock(&cgrp->pidlist_mutex);
 * 	list_for_each_entry_safe(l, tmp_l, &cgrp->pidlists, links)
 * 		mod_delayed_work(cgroup_pidlist_destroy_wq, &l->destroy_dwork, 0);
 * 	mutex_unlock(&cgrp->pidlist_mutex);
 * 
 * 	flush_workqueue(cgroup_pidlist_destroy_wq);
 * 	BUG_ON(!list_empty(&cgrp->pidlists));
 * }
 */
static void cgroup_pidlist_destroy_work_fn(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct cgroup_pidlist *l = container_of(dwork, struct cgroup_pidlist,
						destroy_dwork);
	struct cgroup_pidlist *tofree = NULL;

	mutex_lock(&l->owner->pidlist_mutex);

	/*
	 * Destroy iff we didn't get queued again.  The state won't change
	 * as destroy_dwork can only be queued while locked.
	 */
	if (!delayed_work_pending(dwork)) {
		list_del(&l->links);
		kvfree(l->list);
		put_pid_ns(l->key.ns);
		tofree = l;
	}

	mutex_unlock(&l->owner->pidlist_mutex);
	kfree(tofree);
}
/* Context-After
 * /*
 *  * pidlist_uniq - given a kmalloc()ed list, strip out all duplicate entries
 *  * Returns the number of unique elements.
 *  */
 * static int pidlist_uniq(pid_t *list, int length)
 * {
 * 	int src, dest = 1;
 * 
 * 	/*
 * 	 * we presume the 0th element is unique, so i starts at 1. trivial
 * 	 * edge cases first; no work needs to be done for either
 * 	 */
 * 	if (length == 0 || length == 1)
 * 		return length;
 * 	/* src and dest walk down the list; dest counts unique elements */
 * 	for (src = 1; src < length; src++) {
 * 		/* find next unique element */
 * 		while (list[src] == list[src-1]) {
 * 			src++;
 */
