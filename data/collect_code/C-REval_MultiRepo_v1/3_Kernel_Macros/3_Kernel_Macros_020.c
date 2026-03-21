/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_020 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 11 */
/* NLOC: 42 */
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
 * 	cgroup_attach_unlock(CGRP_ATTACH_LOCK_GLOBAL, NULL);
 * 	cgroup_unlock();
 * 
 * 	return retval;
 * }
 * EXPORT_SYMBOL_GPL(cgroup_attach_task_all);
 * 
 * /**
 *  * cgroup_transfer_tasks - move tasks from one cgroup to another
 *  * @to: cgroup to which the tasks will be moved
 *  * @from: cgroup in which the tasks currently reside
 *  *
 *  * Locking rules between cgroup_post_fork() and the migration path
 *  * guarantee that, if a task is forking while being migrated, the new child
 *  * is guaranteed to be either visible in the source cgroup after the
 *  * parent's migration is complete or put into the target cgroup.  No task
 *  * can slip out of migration through forking.
 *  *
 *  * Return: %0 on success or a negative errno code on failure
 *  */
 */
int cgroup_transfer_tasks(struct cgroup *to, struct cgroup *from)
{
	DEFINE_CGROUP_MGCTX(mgctx);
	struct cgrp_cset_link *link;
	struct css_task_iter it;
	struct task_struct *task;
	int ret;

	if (cgroup_on_dfl(to))
		return -EINVAL;

	ret = cgroup_migrate_vet_dst(to);
	if (ret)
		return ret;

	cgroup_lock();

	cgroup_attach_lock(CGRP_ATTACH_LOCK_GLOBAL, NULL);

	/* all tasks in @from are being moved, all csets are source */
	spin_lock_irq(&css_set_lock);
	list_for_each_entry(link, &from->cset_links, cset_link)
		cgroup_migrate_add_src(link->cset, to, &mgctx);
	spin_unlock_irq(&css_set_lock);

	ret = cgroup_migrate_prepare_dst(&mgctx);
	if (ret)
		goto out_err;

	/*
	 * Migrate tasks one-by-one until @from is empty.  This fails iff
	 * ->can_attach() fails.
	 */
	do {
		css_task_iter_start(&from->self, 0, &it);

		do {
			task = css_task_iter_next(&it);
		} while (task && (task->flags & PF_EXITING));

		if (task)
			get_task_struct(task);
		css_task_iter_end(&it);

		if (task) {
			ret = cgroup_migrate(task, false, &mgctx);
			if (!ret)
				TRACE_CGROUP_PATH(transfer_tasks, to, task, false);
			put_task_struct(task);
		}
	} while (task && !ret);
out_err:
	cgroup_migrate_finish(&mgctx);
	cgroup_attach_unlock(CGRP_ATTACH_LOCK_GLOBAL, NULL);
	cgroup_unlock();
	return ret;
}
/* Context-After
 * /*
 *  * Stuff for reading the 'tasks'/'procs' files.
 *  *
 *  * Reading this file can return large amounts of data if a cgroup has
 *  * *lots* of attached tasks. So it may need several calls to read(),
 *  * but we cannot guarantee that the information we produce is correct
 *  * unless we produce it entirely atomically.
 *  *
 *  */
 * 
 * /* which pidlist file are we talking about? */
 * enum cgroup_filetype {
 * 	CGROUP_FILE_PROCS,
 * 	CGROUP_FILE_TASKS,
 * };
 * 
 * /*
 *  * A pidlist is a list of pids that virtually represents the contents of one
 *  * of the cgroup files ("procs" or "tasks"). We keep a list of such pidlists,
 */
