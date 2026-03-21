/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_021 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 18 */
/* NLOC: 94 */
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
 *  */
 * void cdns3_gadget_ep_free_request(struct usb_ep *ep,
 * 				  struct usb_request *request)
 * {
 * 	struct cdns3_request *priv_req = to_cdns3_request(request);
 * 
 * 	if (priv_req->aligned_buf)
 * 		priv_req->aligned_buf->in_use = 0;
 * 
 * 	trace_cdns3_free_request(priv_req);
 * 	kfree(priv_req);
 * }
 * 
 * /**
 *  * cdns3_gadget_ep_enable - Enable endpoint
 *  * @ep: endpoint object
 *  * @desc: endpoint descriptor
 *  *
 *  * Returns 0 on success, error code elsewhere
 *  */
 */
static int cdns3_gadget_ep_enable(struct usb_ep *ep,
				  const struct usb_endpoint_descriptor *desc)
{
	struct cdns3_endpoint *priv_ep;
	struct cdns3_device *priv_dev;
	const struct usb_ss_ep_comp_descriptor *comp_desc;
	u32 reg = EP_STS_EN_TRBERREN;
	u32 bEndpointAddress;
	unsigned long flags;
	int enable = 1;
	int ret = 0;
	int val;

	if (!ep) {
		pr_debug("usbss: ep not configured?\n");
		return -EINVAL;
	}

	priv_ep = ep_to_cdns3_ep(ep);
	priv_dev = priv_ep->cdns3_dev;
	comp_desc = priv_ep->endpoint.comp_desc;

	if (!desc || desc->bDescriptorType != USB_DT_ENDPOINT) {
		dev_dbg(priv_dev->dev, "usbss: invalid parameters\n");
		return -EINVAL;
	}

	if (!desc->wMaxPacketSize) {
		dev_err(priv_dev->dev, "usbss: missing wMaxPacketSize\n");
		return -EINVAL;
	}

	if (dev_WARN_ONCE(priv_dev->dev, priv_ep->flags & EP_ENABLED,
			  "%s is already enabled\n", priv_ep->name))
		return 0;

	spin_lock_irqsave(&priv_dev->lock, flags);

	priv_ep->endpoint.desc = desc;
	priv_ep->type = usb_endpoint_type(desc);
	priv_ep->interval = desc->bInterval ? BIT(desc->bInterval - 1) : 0;

	if (priv_ep->interval > ISO_MAX_INTERVAL &&
	    priv_ep->type == USB_ENDPOINT_XFER_ISOC) {
		dev_err(priv_dev->dev, "Driver is limited to %d period\n",
			ISO_MAX_INTERVAL);

		ret =  -EINVAL;
		goto exit;
	}

	bEndpointAddress = priv_ep->num | priv_ep->dir;
	cdns3_select_ep(priv_dev, bEndpointAddress);

	/*
	 * For some versions of controller at some point during ISO OUT traffic
	 * DMA reads Transfer Ring for the EP which has never got doorbell.
	 * This issue was detected only on simulation, but to avoid this issue
	 * driver add protection against it. To fix it driver enable ISO OUT
	 * endpoint before setting DRBL. This special treatment of ISO OUT
	 * endpoints are recommended by controller specification.
	 */
	if (priv_ep->type == USB_ENDPOINT_XFER_ISOC  && !priv_ep->dir)
		enable = 0;

	if (usb_ss_max_streams(comp_desc) && usb_endpoint_xfer_bulk(desc)) {
		/*
		 * Enable stream support (SS mode) related interrupts
		 * in EP_STS_EN Register
		 */
		if (priv_dev->gadget.speed >= USB_SPEED_SUPER) {
			reg |= EP_STS_EN_IOTEN | EP_STS_EN_PRIMEEEN |
				EP_STS_EN_SIDERREN | EP_STS_EN_MD_EXITEN |
				EP_STS_EN_STREAMREN;
			priv_ep->use_streams = true;
			ret = cdns3_ep_config(priv_ep, enable);
			priv_dev->using_streams |= true;
		}
	} else {
		ret = cdns3_ep_config(priv_ep, enable);
	}

	if (ret)
		goto exit;

	ret = cdns3_allocate_trb_pool(priv_ep);
	if (ret)
		goto exit;

	bEndpointAddress = priv_ep->num | priv_ep->dir;
	cdns3_select_ep(priv_dev, bEndpointAddress);

	trace_cdns3_gadget_ep_enable(priv_ep);

	writel(EP_CMD_EPRST, &priv_dev->regs->ep_cmd);

	ret = readl_poll_timeout_atomic(&priv_dev->regs->ep_cmd, val,
					!(val & (EP_CMD_CSTALL | EP_CMD_EPRST)),
					1, 1000);

	if (unlikely(ret)) {
		cdns3_free_trb_pool(priv_ep);
		ret =  -EINVAL;
		goto exit;
	}

	/* enable interrupt for selected endpoint */
	cdns3_set_register_bit(&priv_dev->regs->ep_ien,
			       BIT(cdns3_ep_addr_to_index(bEndpointAddress)));

	if (priv_dev->dev_ver < DEV_VER_V2)
		cdns3_wa2_enable_detection(priv_dev, priv_ep, reg);

	writel(reg, &priv_dev->regs->ep_sts_en);

	ep->desc = desc;
	priv_ep->flags &= ~(EP_PENDING_REQUEST | EP_STALLED | EP_STALL_PENDING |
			    EP_QUIRK_ISO_OUT_EN | EP_QUIRK_EXTRA_BUF_EN);
	priv_ep->flags |= EP_ENABLED | EP_UPDATE_EP_TRBADDR;
	priv_ep->wa1_set = 0;
	priv_ep->enqueue = 0;
	priv_ep->dequeue = 0;
	reg = readl(&priv_dev->regs->ep_sts);
	priv_ep->pcs = !!EP_STS_CCS(reg);
	priv_ep->ccs = !!EP_STS_CCS(reg);
	/* one TRB is reserved for link TRB used in DMULT mode*/
	priv_ep->free_trbs = priv_ep->num_trbs - 1;
exit:
	spin_unlock_irqrestore(&priv_dev->lock, flags);

	return ret;
}
/* Context-After
 * /**
 *  * cdns3_gadget_ep_disable - Disable endpoint
 *  * @ep: endpoint object
 *  *
 *  * Returns 0 on success, error code elsewhere
 *  */
 * static int cdns3_gadget_ep_disable(struct usb_ep *ep)
 * {
 * 	struct cdns3_endpoint *priv_ep;
 * 	struct cdns3_request *priv_req;
 * 	struct cdns3_device *priv_dev;
 * 	struct usb_request *request;
 * 	unsigned long flags;
 * 	int ret = 0;
 * 	u32 ep_cfg;
 * 	int val;
 * 
 * 	if (!ep) {
 * 		pr_err("usbss: invalid parameters\n");
 */
