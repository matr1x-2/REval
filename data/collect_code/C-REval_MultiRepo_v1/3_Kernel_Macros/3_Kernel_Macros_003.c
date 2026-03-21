/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_003 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 5 */
/* NLOC: 16 */
/* Subsystem: net */
/* Includes
 * #include <linux/netdevice.h>
 * #include <linux/netlink.h>
 * #include <linux/slab.h>
 * #include <net/netlink.h>
 * #include <net/rtnetlink.h>
 * #include <linux/dcbnl.h>
 * #include <net/dcbevent.h>
 * #include <linux/rtnetlink.h>
 * #include <linux/init.h>
 * #include <net/sock.h>
 */
/* Context-Before
 * 		if (itr->ifindex == ifindex &&
 * 		    itr->app.selector == IEEE_8021QAZ_APP_SEL_DSCP &&
 * 		    itr->app.protocol < 64 &&
 * 		    itr->app.priority < IEEE_8021QAZ_MAX_TCS)
 * 			p_map->map[itr->app.protocol] |= 1 << itr->app.priority;
 * 	}
 * 	spin_unlock_bh(&dcb_lock);
 * }
 * EXPORT_SYMBOL(dcb_ieee_getapp_dscp_prio_mask_map);
 * 
 * /*
 *  * Per 802.1Q-2014, the selector value of 1 is used for matching on Ethernet
 *  * type, with valid PID values >= 1536. A special meaning is then assigned to
 *  * protocol value of 0: "default priority. For use when priority is not
 *  * otherwise specified".
 *  *
 *  * dcb_ieee_getapp_default_prio_mask - For a given device, find all APP entries
 *  * of the form {$PRIO, ETHERTYPE, 0} and construct a bit mask of all default
 *  * priorities set by these entries.
 *  */
 */
u8 dcb_ieee_getapp_default_prio_mask(const struct net_device *dev)
{
	int ifindex = dev->ifindex;
	struct dcb_app_type *itr;
	u8 mask = 0;

	spin_lock_bh(&dcb_lock);
	list_for_each_entry(itr, &dcb_app_list, list) {
		if (itr->ifindex == ifindex &&
		    itr->app.selector == IEEE_8021QAZ_APP_SEL_ETHERTYPE &&
		    itr->app.protocol == 0 &&
		    itr->app.priority < IEEE_8021QAZ_MAX_TCS)
			mask |= 1 << itr->app.priority;
	}
	spin_unlock_bh(&dcb_lock);

	return mask;
}
/* Context-After
 * EXPORT_SYMBOL(dcb_ieee_getapp_default_prio_mask);
 * 
 * static void dcbnl_flush_dev(struct net_device *dev)
 * {
 * 	struct dcb_app_type *itr, *tmp;
 * 
 * 	spin_lock_bh(&dcb_lock);
 * 
 * 	list_for_each_entry_safe(itr, tmp, &dcb_app_list, list) {
 * 		if (itr->ifindex == dev->ifindex) {
 * 			list_del(&itr->list);
 * 			kfree(itr);
 * 		}
 * 	}
 * 
 * 	spin_unlock_bh(&dcb_lock);
 * }
 * 
 * static int dcbnl_netdevice_event(struct notifier_block *nb,
 * 				 unsigned long event, void *ptr)
 */
