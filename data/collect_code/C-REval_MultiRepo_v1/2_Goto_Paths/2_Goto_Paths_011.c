/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_011 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 18 */
/* NLOC: 74 */
/* Subsystem: net */
/* Includes
 * #include <linux/module.h>
 * #include <linux/init.h>
 * #include <linux/kernel.h>
 * #include <linux/list.h>
 * #include <linux/netdevice.h>
 * #include <linux/skbuff.h>
 * #include <linux/etherdevice.h>
 * #include <linux/rtnetlink.h>
 * #include <linux/ip.h>
 * #include <linux/uaccess.h>
 * #include <linux/slab.h>
 * #include <net/arp.h>
 * #include <linux/atm.h>
 * #include <linux/atmdev.h>
 * #include <linux/capability.h>
 * #include <linux/seq_file.h>
 * #include <linux/atmbr2684.h>
 * #include "common.h"
 */
/* Context-Before
 * 		goto dropped;
 * 	net_dev->stats.rx_packets++;
 * 	net_dev->stats.rx_bytes += skb->len;
 * 	memset(ATM_SKB(skb), 0, sizeof(struct atm_skb_data));
 * 	netif_rx(skb);
 * 	return;
 * 
 * dropped:
 * 	net_dev->stats.rx_dropped++;
 * 	goto free_skb;
 * error:
 * 	net_dev->stats.rx_errors++;
 * free_skb:
 * 	dev_kfree_skb(skb);
 * }
 * 
 * /*
 *  * Assign a vcc to a dev
 *  * Note: we do not have explicit unassign, but look at _push()
 *  */
 */
static int br2684_regvcc(struct atm_vcc *atmvcc, void __user * arg)
{
	struct br2684_vcc *brvcc;
	struct br2684_dev *brdev;
	struct net_device *net_dev;
	struct atm_backend_br2684 be;
	int err;

	if (copy_from_user(&be, arg, sizeof be))
		return -EFAULT;
	brvcc = kzalloc_obj(struct br2684_vcc);
	if (!brvcc)
		return -ENOMEM;
	/*
	 * Allow two packets in the ATM queue. One actually being sent, and one
	 * for the ATM 'TX done' handler to send. It shouldn't take long to get
	 * the next one from the netdev queue, when we need it. More than that
	 * would be bufferbloat.
	 */
	atomic_set(&brvcc->qspace, 2);
	write_lock_irq(&devs_lock);
	net_dev = br2684_find_dev(&be.ifspec);
	if (net_dev == NULL) {
		pr_err("tried to attach to non-existent device\n");
		err = -ENXIO;
		goto error;
	}
	brdev = BRPRIV(net_dev);
	if (atmvcc->push == NULL) {
		err = -EBADFD;
		goto error;
	}
	if (!list_empty(&brdev->brvccs)) {
		/* Only 1 VCC/dev right now */
		err = -EEXIST;
		goto error;
	}
	if (be.fcs_in != BR2684_FCSIN_NO ||
	    be.fcs_out != BR2684_FCSOUT_NO ||
	    be.fcs_auto || be.has_vpiid || be.send_padding ||
	    (be.encaps != BR2684_ENCAPS_VC &&
	     be.encaps != BR2684_ENCAPS_LLC) ||
	    be.min_size != 0) {
		err = -EINVAL;
		goto error;
	}
	pr_debug("vcc=%p, encaps=%d, brvcc=%p\n", atmvcc, be.encaps, brvcc);
	if (list_empty(&brdev->brvccs) && !brdev->mac_was_set) {
		unsigned char *esi = atmvcc->dev->esi;
		const u8 one = 1;

		if (esi[0] | esi[1] | esi[2] | esi[3] | esi[4] | esi[5])
			dev_addr_set(net_dev, esi);
		else
			dev_addr_mod(net_dev, 2, &one, 1);
	}
	list_add(&brvcc->brvccs, &brdev->brvccs);
	write_unlock_irq(&devs_lock);
	brvcc->device = net_dev;
	brvcc->atmvcc = atmvcc;
	atmvcc->user_back = brvcc;
	brvcc->encaps = (enum br2684_encaps)be.encaps;
	brvcc->old_push = atmvcc->push;
	brvcc->old_pop = atmvcc->pop;
	brvcc->old_release_cb = atmvcc->release_cb;
	brvcc->old_owner = atmvcc->owner;
	barrier();
	atmvcc->push = br2684_push;
	atmvcc->pop = br2684_pop;
	atmvcc->release_cb = br2684_release_cb;
	atmvcc->owner = THIS_MODULE;

	/* initialize netdev carrier state */
	if (atmvcc->dev->signal == ATM_PHY_SIG_LOST)
		netif_carrier_off(net_dev);
	else
		netif_carrier_on(net_dev);

	__module_get(THIS_MODULE);

	/* re-process everything received between connection setup and
	   backend setup */
	vcc_process_recv_queue(atmvcc);
	return 0;

error:
	write_unlock_irq(&devs_lock);
	kfree(brvcc);
	return err;
}
/* Context-After
 * static const struct net_device_ops br2684_netdev_ops = {
 * 	.ndo_start_xmit 	= br2684_start_xmit,
 * 	.ndo_set_mac_address	= br2684_mac_addr,
 * 	.ndo_validate_addr	= eth_validate_addr,
 * };
 * 
 * static const struct net_device_ops br2684_netdev_ops_routed = {
 * 	.ndo_start_xmit 	= br2684_start_xmit,
 * 	.ndo_set_mac_address	= br2684_mac_addr,
 * };
 * 
 * static void br2684_setup(struct net_device *netdev)
 * {
 * 	struct br2684_dev *brdev = BRPRIV(netdev);
 * 
 * 	ether_setup(netdev);
 * 	netdev->hard_header_len += sizeof(llc_oui_pid_pad); /* worst case */
 * 	brdev->net_dev = netdev;
 */
