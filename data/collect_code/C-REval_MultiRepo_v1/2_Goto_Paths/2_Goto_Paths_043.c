/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_043 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 29 */
/* Subsystem: arch */
/* Includes
 * #include <linux/errno.h>
 * #include <linux/fs.h>
 * #include <linux/init.h>
 * #include <linux/module.h>
 * #include <linux/moduleparam.h>
 * #include <linux/gfp.h>
 * #include <linux/sched.h>
 * #include <linux/string_helpers.h>
 * #include <linux/sysctl.h>
 * #include <linux/swap.h>
 * #include <linux/kthread.h>
 * #include <linux/oom.h>
 * #include <linux/uaccess.h>
 * #include <asm/diag.h>
 * #include "../../../drivers/s390/net/smsgiucv.h"
 */
/* Context-Before
 * 		nr = simple_strtoul(msg, &msg, 0);
 * 		cmm_skip_blanks(msg, &msg);
 * 		if (*msg == '\0')
 * 			cmm_add_timed_pages(nr);
 * 	} else if (strncmp(msg, "REUSE", 5) == 0) {
 * 		if (!cmm_skip_blanks(msg + 5, &msg))
 * 			return;
 * 		nr = simple_strtoul(msg, &msg, 0);
 * 		if (!cmm_skip_blanks(msg, &msg))
 * 			return;
 * 		seconds = simple_strtoul(msg, &msg, 0);
 * 		cmm_skip_blanks(msg, &msg);
 * 		if (*msg == '\0')
 * 			cmm_set_timeout(nr, seconds);
 * 	}
 * }
 * #endif
 * 
 * static struct ctl_table_header *cmm_sysctl_header;
 */
static int __init cmm_init(void)
{
	int rc = -ENOMEM;

	cmm_sysctl_header = register_sysctl("vm", cmm_table);
	if (!cmm_sysctl_header)
		goto out_sysctl;
#ifdef CONFIG_CMM_IUCV
	/* convert sender to uppercase characters */
	if (sender)
		string_upper(sender, sender);
	else
		sender = cmm_default_sender;

	rc = smsg_register_callback(SMSG_PREFIX, cmm_smsg_target);
	if (rc < 0)
		goto out_smsg;
#endif
	rc = register_oom_notifier(&cmm_oom_nb);
	if (rc < 0)
		goto out_oom_notify;
	cmm_thread_ptr = kthread_run(cmm_thread, NULL, "cmmthread");
	if (!IS_ERR(cmm_thread_ptr))
		return 0;

	rc = PTR_ERR(cmm_thread_ptr);
	unregister_oom_notifier(&cmm_oom_nb);
out_oom_notify:
#ifdef CONFIG_CMM_IUCV
	smsg_unregister_callback(SMSG_PREFIX, cmm_smsg_target);
out_smsg:
#endif
	unregister_sysctl_table(cmm_sysctl_header);
out_sysctl:
	timer_delete_sync(&cmm_timer);
	return rc;
}
/* Context-After
 * module_init(cmm_init);
 * 
 * static void __exit cmm_exit(void)
 * {
 * 	unregister_sysctl_table(cmm_sysctl_header);
 * #ifdef CONFIG_CMM_IUCV
 * 	smsg_unregister_callback(SMSG_PREFIX, cmm_smsg_target);
 * #endif
 * 	unregister_oom_notifier(&cmm_oom_nb);
 * 	kthread_stop(cmm_thread_ptr);
 * 	timer_delete_sync(&cmm_timer);
 * 	cmm_free_pages(cmm_pages, &cmm_pages, &cmm_page_list);
 * 	cmm_free_pages(cmm_timed_pages, &cmm_timed_pages, &cmm_timed_page_list);
 * }
 * module_exit(cmm_exit);
 * 
 * MODULE_DESCRIPTION("Cooperative memory management interface");
 * MODULE_LICENSE("GPL");
 */
