/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_017 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 18 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/dma-mapping.h>
 * #include <linux/usb/gadget.h>
 * #include <linux/module.h>
 * #include <linux/dmapool.h>
 * #include <linux/iopoll.h>
 * #include <linux/property.h>
 * #include "core.h"
 * #include "gadget-export.h"
 * #include "cdns3-gadget.h"
 * #include "cdns3-trace.h"
 * #include "drd.h"
 */
/* Context-Before
 * 		priv_ep->flags &= ~EP_UPDATE_EP_TRBADDR;
 * 	}
 * 
 * 	if (!priv_ep->wa1_set && !(priv_ep->flags & EP_STALLED)) {
 * 		trace_cdns3_ring(priv_ep);
 * 		/*clearing TRBERR and EP_STS_DESCMIS before seting DRDY*/
 * 		writel(EP_STS_TRBERR | EP_STS_DESCMIS, &priv_dev->regs->ep_sts);
 * 		writel(EP_CMD_DRDY, &priv_dev->regs->ep_cmd);
 * 		cdns3_rearm_drdy_if_needed(priv_ep);
 * 		trace_cdns3_doorbell_epx(priv_ep->name,
 * 					 readl(&priv_dev->regs->ep_traddr));
 * 	}
 * 
 * 	/* WORKAROUND for transition to L0 */
 * 	__cdns3_gadget_wakeup(priv_dev);
 * 
 * 	return 0;
 * }
 */
void cdns3_set_hw_configuration(struct cdns3_device *priv_dev)
{
	struct cdns3_endpoint *priv_ep;
	struct usb_ep *ep;

	if (priv_dev->hw_configured_flag)
		return;

	writel(USB_CONF_CFGSET, &priv_dev->regs->usb_conf);

	cdns3_set_register_bit(&priv_dev->regs->usb_conf,
			       USB_CONF_U1EN | USB_CONF_U2EN);

	priv_dev->hw_configured_flag = 1;

	list_for_each_entry(ep, &priv_dev->gadget.ep_list, ep_list) {
		if (ep->enabled) {
			priv_ep = ep_to_cdns3_ep(ep);
			cdns3_start_all_request(priv_dev, priv_ep);
		}
	}

	cdns3_allow_enable_l1(priv_dev, 1);
}
/* Context-After
 * /**
 *  * cdns3_trb_handled - check whether trb has been handled by DMA
 *  *
 *  * @priv_ep: extended endpoint object.
 *  * @priv_req: request object for checking
 *  *
 *  * Endpoint must be selected before invoking this function.
 *  *
 *  * Returns false if request has not been handled by DMA, else returns true.
 *  *
 *  * SR - start ring
 *  * ER -  end ring
 *  * DQ = priv_ep->dequeue - dequeue position
 *  * EQ = priv_ep->enqueue -  enqueue position
 *  * ST = priv_req->start_trb - index of first TRB in transfer ring
 *  * ET = priv_req->end_trb - index of last TRB in transfer ring
 *  * CI = current_index - index of processed TRB by DMA.
 *  *
 *  * As first step, we check if the TRB between the ST and ET.
 */
