/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_013 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 53 */
/* Subsystem: net */
/* Includes
 * #include <linux/list.h>
 * #include <linux/etherdevice.h>
 * #include <linux/netdevice.h>
 * #include <linux/phy.h>
 * #include <linux/phy_fixed.h>
 * #include <linux/phylink.h>
 * #include <linux/of_net.h>
 * #include <linux/of_mdio.h>
 * #include <linux/mdio.h>
 * #include <net/rtnetlink.h>
 * #include <net/pkt_cls.h>
 * #include <net/selftests.h>
 * #include <net/tc_act/tc_mirred.h>
 * #include <linux/if_bridge.h>
 * #include <linux/if_hsr.h>
 * #include <net/dcbnl.h>
 * #include <linux/netpoll.h>
 * #include <linux/string.h>
 * #include "conduit.h"
 * #include "dsa.h"
 */
/* Context-Before
 * struct dsa_host_vlan_rx_filtering_ctx {
 * 	struct net_device *dev;
 * 	const unsigned char *addr;
 * 	enum dsa_standalone_event event;
 * };
 * 
 * static bool dsa_switch_supports_uc_filtering(struct dsa_switch *ds)
 * {
 * 	return ds->ops->port_fdb_add && ds->ops->port_fdb_del &&
 * 	       ds->fdb_isolation && !ds->vlan_filtering_is_global &&
 * 	       !ds->needs_standalone_vlan_filtering;
 * }
 * 
 * static bool dsa_switch_supports_mc_filtering(struct dsa_switch *ds)
 * {
 * 	return ds->ops->port_mdb_add && ds->ops->port_mdb_del &&
 * 	       ds->fdb_isolation && !ds->vlan_filtering_is_global &&
 * 	       !ds->needs_standalone_vlan_filtering;
 * }
 */
static void dsa_user_standalone_event_work(struct work_struct *work)
{
	struct dsa_standalone_event_work *standalone_work =
		container_of(work, struct dsa_standalone_event_work, work);
	const unsigned char *addr = standalone_work->addr;
	struct net_device *dev = standalone_work->dev;
	struct dsa_port *dp = dsa_user_to_port(dev);
	struct switchdev_obj_port_mdb mdb;
	struct dsa_switch *ds = dp->ds;
	u16 vid = standalone_work->vid;
	int err;

	switch (standalone_work->event) {
	case DSA_UC_ADD:
		err = dsa_port_standalone_host_fdb_add(dp, addr, vid);
		if (err) {
			dev_err(ds->dev,
				"port %d failed to add %pM vid %d to fdb: %d\n",
				dp->index, addr, vid, err);
			break;
		}
		break;

	case DSA_UC_DEL:
		err = dsa_port_standalone_host_fdb_del(dp, addr, vid);
		if (err) {
			dev_err(ds->dev,
				"port %d failed to delete %pM vid %d from fdb: %d\n",
				dp->index, addr, vid, err);
		}

		break;
	case DSA_MC_ADD:
		ether_addr_copy(mdb.addr, addr);
		mdb.vid = vid;

		err = dsa_port_standalone_host_mdb_add(dp, &mdb);
		if (err) {
			dev_err(ds->dev,
				"port %d failed to add %pM vid %d to mdb: %d\n",
				dp->index, addr, vid, err);
			break;
		}
		break;
	case DSA_MC_DEL:
		ether_addr_copy(mdb.addr, addr);
		mdb.vid = vid;

		err = dsa_port_standalone_host_mdb_del(dp, &mdb);
		if (err) {
			dev_err(ds->dev,
				"port %d failed to delete %pM vid %d from mdb: %d\n",
				dp->index, addr, vid, err);
		}

		break;
	}

	kfree(standalone_work);
}
/* Context-After
 * static int dsa_user_schedule_standalone_work(struct net_device *dev,
 * 					     enum dsa_standalone_event event,
 * 					     const unsigned char *addr,
 * 					     u16 vid)
 * {
 * 	struct dsa_standalone_event_work *standalone_work;
 * 
 * 	standalone_work = kzalloc_obj(*standalone_work, GFP_ATOMIC);
 * 	if (!standalone_work)
 * 		return -ENOMEM;
 * 
 * 	INIT_WORK(&standalone_work->work, dsa_user_standalone_event_work);
 * 	standalone_work->event = event;
 * 	standalone_work->dev = dev;
 * 
 * 	ether_addr_copy(standalone_work->addr, addr);
 * 	standalone_work->vid = vid;
 * 
 * 	dsa_schedule_work(&standalone_work->work);
 */
