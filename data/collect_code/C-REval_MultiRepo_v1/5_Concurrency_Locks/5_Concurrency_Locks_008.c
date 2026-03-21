/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_008 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 20 */
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
 * out_finish:
 * 	cgroup_procs_write_finish(task, lock_mode);
 * out_unlock:
 * 	cgroup_kn_unlock(of->kn);
 * 
 * 	return ret ?: nbytes;
 * }
 * 
 * static ssize_t cgroup1_procs_write(struct kernfs_open_file *of,
 * 				   char *buf, size_t nbytes, loff_t off)
 * {
 * 	return __cgroup1_procs_write(of, buf, nbytes, off, true);
 * }
 * 
 * static ssize_t cgroup1_tasks_write(struct kernfs_open_file *of,
 * 				   char *buf, size_t nbytes, loff_t off)
 * {
 * 	return __cgroup1_procs_write(of, buf, nbytes, off, false);
 * }
 */
static ssize_t cgroup_release_agent_write(struct kernfs_open_file *of,
					  char *buf, size_t nbytes, loff_t off)
{
	struct cgroup *cgrp;
	struct cgroup_file_ctx *ctx;

	BUILD_BUG_ON(sizeof(cgrp->root->release_agent_path) < PATH_MAX);

	/*
	 * Release agent gets called with all capabilities,
	 * require capabilities to set release agent.
	 */
	ctx = of->priv;
	if ((ctx->ns->user_ns != &init_user_ns) ||
	    !file_ns_capable(of->file, &init_user_ns, CAP_SYS_ADMIN))
		return -EPERM;

	cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!cgrp)
		return -ENODEV;
	spin_lock(&release_agent_path_lock);
	strscpy(cgrp->root->release_agent_path, strstrip(buf),
		sizeof(cgrp->root->release_agent_path));
	spin_unlock(&release_agent_path_lock);
	cgroup_kn_unlock(of->kn);
	return nbytes;
}
/* Context-After
 * static int cgroup_release_agent_show(struct seq_file *seq, void *v)
 * {
 * 	struct cgroup *cgrp = seq_css(seq)->cgroup;
 * 
 * 	spin_lock(&release_agent_path_lock);
 * 	seq_puts(seq, cgrp->root->release_agent_path);
 * 	spin_unlock(&release_agent_path_lock);
 * 	seq_putc(seq, '\n');
 * 	return 0;
 * }
 * 
 * static int cgroup_sane_behavior_show(struct seq_file *seq, void *v)
 * {
 * 	seq_puts(seq, "0\n");
 * 	return 0;
 * }
 * 
 * static u64 cgroup_read_notify_on_release(struct cgroup_subsys_state *css,
 * 					 struct cftype *cft)
 */
