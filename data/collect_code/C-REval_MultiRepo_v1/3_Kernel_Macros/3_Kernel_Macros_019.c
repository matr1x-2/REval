/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_019 */
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
 * 	list_for_each_entry(itr, &dcb_app_list, list) {
 * 		if (itr->ifindex == ifindex &&
 * 		    itr->app.selector == IEEE_8021QAZ_APP_SEL_DSCP &&
 * 		    itr->app.protocol < 64 &&
 * 		    itr->app.priority < IEEE_8021QAZ_MAX_TCS) {
 * 			prio = itr->app.priority;
 * 			p_map->map[prio] |= 1ULL << itr->app.protocol;
 * 		}
 * 	}
 * 	spin_unlock_bh(&dcb_lock);
 * }
 * EXPORT_SYMBOL(dcb_ieee_getapp_prio_dscp_mask_map);
 * 
 * /*
 *  * dcb_ieee_getapp_dscp_prio_mask_map - For a given device, find mapping from
 *  * DSCP values to the priorities assigned to that DSCP value. Initialize p_map
 *  * such that each map element holds a bit mask of priorities configured for a
 *  * given DSCP value by APP entries.
 *  */
 * void
 */
dcb_ieee_getapp_dscp_prio_mask_map(const struct net_device *dev,
				   struct dcb_ieee_app_dscp_map *p_map)
{
	int ifindex = dev->ifindex;
	struct dcb_app_type *itr;

	memset(p_map->map, 0, sizeof(p_map->map));

	spin_lock_bh(&dcb_lock);
	list_for_each_entry(itr, &dcb_app_list, list) {
		if (itr->ifindex == ifindex &&
		    itr->app.selector == IEEE_8021QAZ_APP_SEL_DSCP &&
		    itr->app.protocol < 64 &&
		    itr->app.priority < IEEE_8021QAZ_MAX_TCS)
			p_map->map[itr->app.protocol] |= 1 << itr->app.priority;
	}
	spin_unlock_bh(&dcb_lock);
}
/* Context-After
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
 * u8 dcb_ieee_getapp_default_prio_mask(const struct net_device *dev)
 * {
 * 	int ifindex = dev->ifindex;
 * 	struct dcb_app_type *itr;
 * 	u8 mask = 0;
 * 
 * 	spin_lock_bh(&dcb_lock);
 * 	list_for_each_entry(itr, &dcb_app_list, list) {
 */
