/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_009 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 11 */
/* NLOC: 42 */
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
 * {
 * 	struct rps_dev_flow_table *flow_table;
 * 	unsigned long val = 0;
 * 
 * 	rcu_read_lock();
 * 	flow_table = rcu_dereference(queue->rps_flow_table);
 * 	if (flow_table)
 * 		val = 1UL << flow_table->log;
 * 	rcu_read_unlock();
 * 
 * 	return sysfs_emit(buf, "%lu\n", val);
 * }
 * 
 * static void rps_dev_flow_table_release(struct rcu_head *rcu)
 * {
 * 	struct rps_dev_flow_table *table = container_of(rcu,
 * 	    struct rps_dev_flow_table, rcu);
 * 	vfree(table);
 * }
 */
static ssize_t store_rps_dev_flow_table_cnt(struct netdev_rx_queue *queue,
					    const char *buf, size_t len)
{
	unsigned long mask, count;
	struct rps_dev_flow_table *table, *old_table;
	static DEFINE_SPINLOCK(rps_dev_flow_lock);
	int rc;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	rc = kstrtoul(buf, 0, &count);
	if (rc < 0)
		return rc;

	if (count) {
		mask = count - 1;
		/* mask = roundup_pow_of_two(count) - 1;
		 * without overflows...
		 */
		while ((mask | (mask >> 1)) != mask)
			mask |= (mask >> 1);
		/* On 64 bit arches, must check mask fits in table->mask (u32),
		 * and on 32bit arches, must check
		 * RPS_DEV_FLOW_TABLE_SIZE(mask + 1) doesn't overflow.
		 */
#if BITS_PER_LONG > 32
		if (mask > (unsigned long)(u32)mask)
			return -EINVAL;
#else
		if (mask > (ULONG_MAX - RPS_DEV_FLOW_TABLE_SIZE(1))
				/ sizeof(struct rps_dev_flow)) {
			/* Enforce a limit to prevent overflow */
			return -EINVAL;
		}
#endif
		table = vmalloc(RPS_DEV_FLOW_TABLE_SIZE(mask + 1));
		if (!table)
			return -ENOMEM;

		table->log = ilog2(mask) + 1;
		for (count = 0; count <= mask; count++) {
			table->flows[count].cpu = RPS_NO_CPU;
			table->flows[count].filter = RPS_NO_FILTER;
		}
	} else {
		table = NULL;
	}

	spin_lock(&rps_dev_flow_lock);
	old_table = rcu_dereference_protected(queue->rps_flow_table,
					      lockdep_is_held(&rps_dev_flow_lock));
	rcu_assign_pointer(queue->rps_flow_table, table);
	spin_unlock(&rps_dev_flow_lock);

	if (old_table)
		call_rcu(&old_table->rcu, rps_dev_flow_table_release);

	return len;
}
/* Context-After
 * static struct rx_queue_attribute rps_cpus_attribute __ro_after_init
 * 	= __ATTR(rps_cpus, 0644, show_rps_map, store_rps_map);
 * 
 * static struct rx_queue_attribute rps_dev_flow_table_cnt_attribute __ro_after_init
 * 	= __ATTR(rps_flow_cnt, 0644,
 * 		 show_rps_dev_flow_table_cnt, store_rps_dev_flow_table_cnt);
 * #endif /* CONFIG_RPS */
 * 
 * static struct attribute *rx_queue_default_attrs[] __ro_after_init = {
 * #ifdef CONFIG_RPS
 * 	&rps_cpus_attribute.attr,
 * 	&rps_dev_flow_table_cnt_attribute.attr,
 * #endif
 * 	NULL
 * };
 * ATTRIBUTE_GROUPS(rx_queue_default);
 * 
 * static void rx_queue_release(struct kobject *kobj)
 * {
 */
