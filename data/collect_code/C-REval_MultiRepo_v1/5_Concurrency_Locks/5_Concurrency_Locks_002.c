/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_002 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 20 */
/* Subsystem: net */
/* Includes
 * #include <linux/module.h>
 * #include <linux/spinlock.h>
 * #include <linux/interrupt.h>
 * #include <asm/string.h>
 * #include <linux/kmod.h>
 * #include <linux/sysctl.h>
 * #include <net/ip_vs.h>
 */
/* Context-Before
 * }
 * 
 * /* Lookup pe and try to load it if it doesn't exist */
 * struct ip_vs_pe *ip_vs_pe_getbyname(const char *name)
 * {
 * 	struct ip_vs_pe *pe;
 * 
 * 	/* Search for the pe by name */
 * 	pe = __ip_vs_pe_getbyname(name);
 * 
 * 	/* If pe not found, load the module and search again */
 * 	if (!pe) {
 * 		request_module("ip_vs_pe_%s", name);
 * 		pe = __ip_vs_pe_getbyname(name);
 * 	}
 * 
 * 	return pe;
 * }
 * 
 * /* Register a pe in the pe list */
 */
int register_ip_vs_pe(struct ip_vs_pe *pe)
{
	struct ip_vs_pe *tmp;

	/* increase the module use count */
	if (!ip_vs_use_count_inc())
		return -ENOENT;

	mutex_lock(&ip_vs_pe_mutex);
	/* Make sure that the pe with this name doesn't exist
	 * in the pe list.
	 */
	list_for_each_entry(tmp, &ip_vs_pe, n_list) {
		if (strcmp(tmp->name, pe->name) == 0) {
			mutex_unlock(&ip_vs_pe_mutex);
			ip_vs_use_count_dec();
			pr_err("%s(): [%s] pe already existed "
			       "in the system\n", __func__, pe->name);
			return -EINVAL;
		}
	}
	/* Add it into the d-linked pe list */
	list_add_rcu(&pe->n_list, &ip_vs_pe);
	mutex_unlock(&ip_vs_pe_mutex);

	pr_info("[%s] pe registered.\n", pe->name);

	return 0;
}
/* Context-After
 * EXPORT_SYMBOL_GPL(register_ip_vs_pe);
 * 
 * /* Unregister a pe from the pe list */
 * int unregister_ip_vs_pe(struct ip_vs_pe *pe)
 * {
 * 	mutex_lock(&ip_vs_pe_mutex);
 * 	/* Remove it from the d-linked pe list */
 * 	list_del_rcu(&pe->n_list);
 * 	mutex_unlock(&ip_vs_pe_mutex);
 * 
 * 	/* decrease the module use count */
 * 	ip_vs_use_count_dec();
 * 
 * 	pr_info("[%s] pe unregistered.\n", pe->name);
 * 
 * 	return 0;
 * }
 * EXPORT_SYMBOL_GPL(unregister_ip_vs_pe);
 */
