/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_028 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 5 */
/* NLOC: 22 */
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
 * 			spin_lock(&cmm_lock);
 * 			pa = *list;
 * 			if (!pa || pa->index >= CMM_NR_PAGES) {
 * 				npa->next = pa;
 * 				npa->index = 0;
 * 				pa = npa;
 * 				*list = pa;
 * 			} else
 * 				free_page((unsigned long) npa);
 * 		}
 * 		diag10_range(virt_to_pfn((void *)addr), 1);
 * 		pa->pages[pa->index++] = addr;
 * 		(*counter)++;
 * 		spin_unlock(&cmm_lock);
 * 		nr--;
 * 		cond_resched();
 * 	}
 * 	return nr;
 * }
 */
static long __cmm_free_pages(long nr, long *counter, struct cmm_page_array **list)
{
	struct cmm_page_array *pa;
	unsigned long addr;

	spin_lock(&cmm_lock);
	pa = *list;
	while (nr) {
		if (!pa || pa->index <= 0)
			break;
		addr = pa->pages[--pa->index];
		if (pa->index == 0) {
			pa = pa->next;
			free_page((unsigned long) *list);
			*list = pa;
		}
		free_page(addr);
		(*counter)--;
		nr--;
	}
	spin_unlock(&cmm_lock);
	return nr;
}
/* Context-After
 * static long cmm_free_pages(long nr, long *counter, struct cmm_page_array **list)
 * {
 * 	long inc = 0;
 * 
 * 	while (nr) {
 * 		inc = min(256L, nr);
 * 		nr -= inc;
 * 		inc = __cmm_free_pages(inc, counter, list);
 * 		if (inc)
 * 			break;
 * 		cond_resched();
 * 	}
 * 	return nr + inc;
 * }
 * 
 * static int cmm_oom_notify(struct notifier_block *self,
 * 			  unsigned long dummy, void *parm)
 * {
 * 	unsigned long *freed = parm;
 */
