/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_015 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 33 */
/* Subsystem: net */
/* Includes
 * #include <linux/capability.h>
 * #include <linux/kernel.h>
 * #include <linux/netdevice.h>
 * #include <linux/if_arp.h>
 * #include <linux/slab.h>
 * #include <linux/sched/signal.h>
 * #include <linux/sched/isolation.h>
 * #include <linux/nsproxy.h>
 * #include <net/sock.h>
 * #include <net/net_namespace.h>
 * #include <linux/rtnetlink.h>
 * #include <linux/vmalloc.h>
 * #include <linux/export.h>
 * #include <linux/jiffies.h>
 * #include <linux/pm_runtime.h>
 * #include <linux/of.h>
 * #include <linux/of_net.h>
 * #include <linux/cpu.h>
 * #include <net/netdev_lock.h>
 * #include <net/netdev_rx_queue.h>
 */
/* Context-Before
 * 	struct rps_map *map;
 * 	cpumask_var_t mask;
 * 	int i, len;
 * 
 * 	if (!zalloc_cpumask_var(&mask, GFP_KERNEL))
 * 		return -ENOMEM;
 * 
 * 	rcu_read_lock();
 * 	map = rcu_dereference(queue->rps_map);
 * 	if (map)
 * 		for (i = 0; i < map->len; i++)
 * 			cpumask_set_cpu(map->cpus[i], mask);
 * 
 * 	len = sysfs_emit(buf, "%*pb\n", cpumask_pr_args(mask));
 * 	rcu_read_unlock();
 * 	free_cpumask_var(mask);
 * 
 * 	return len < PAGE_SIZE ? len : -EINVAL;
 * }
 */
static int netdev_rx_queue_set_rps_mask(struct netdev_rx_queue *queue,
					cpumask_var_t mask)
{
	static DEFINE_MUTEX(rps_map_mutex);
	struct rps_map *old_map, *map;
	int cpu, i;

	map = kzalloc(max_t(unsigned int,
			    RPS_MAP_SIZE(cpumask_weight(mask)), L1_CACHE_BYTES),
		      GFP_KERNEL);
	if (!map)
		return -ENOMEM;

	i = 0;
	for_each_cpu_and(cpu, mask, cpu_online_mask)
		map->cpus[i++] = cpu;

	if (i) {
		map->len = i;
	} else {
		kfree(map);
		map = NULL;
	}

	mutex_lock(&rps_map_mutex);
	old_map = rcu_dereference_protected(queue->rps_map,
					    mutex_is_locked(&rps_map_mutex));
	rcu_assign_pointer(queue->rps_map, map);

	if (map)
		static_branch_inc(&rps_needed);
	if (old_map)
		static_branch_dec(&rps_needed);

	mutex_unlock(&rps_map_mutex);

	if (old_map)
		kfree_rcu(old_map, rcu);
	return 0;
}
/* Context-After
 * int rps_cpumask_housekeeping(struct cpumask *mask)
 * {
 * 	if (!cpumask_empty(mask)) {
 * 		cpumask_and(mask, mask, housekeeping_cpumask(HK_TYPE_DOMAIN_BOOT));
 * 		cpumask_and(mask, mask, housekeeping_cpumask(HK_TYPE_WQ));
 * 		if (cpumask_empty(mask))
 * 			return -EINVAL;
 * 	}
 * 	return 0;
 * }
 * 
 * static ssize_t store_rps_map(struct netdev_rx_queue *queue,
 * 			     const char *buf, size_t len)
 * {
 * 	cpumask_var_t mask;
 * 	int err;
 * 
 * 	if (!capable(CAP_NET_ADMIN))
 * 		return -EPERM;
 */
