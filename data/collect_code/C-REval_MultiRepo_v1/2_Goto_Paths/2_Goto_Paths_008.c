/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_008 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 24 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/circ_buf.h>
 * #include <linux/clk-provider.h>
 * #include <linux/clk.h>
 * #include <linux/crc32.h>
 * #include <linux/dma-mapping.h>
 * #include <linux/etherdevice.h>
 * #include <linux/firmware/xlnx-zynqmp.h>
 * #include <linux/inetdevice.h>
 * #include <linux/init.h>
 * #include <linux/interrupt.h>
 * #include <linux/io.h>
 * #include <linux/iopoll.h>
 * #include <linux/ip.h>
 * #include <linux/kernel.h>
 * #include <linux/module.h>
 * #include <linux/moduleparam.h>
 * #include <linux/netdevice.h>
 * #include <linux/of.h>
 * #include <linux/of_mdio.h>
 * #include <linux/of_net.h>
 */
/* Context-Before
 * 		addr[5] = (top >> 8) & 0xff;
 * 
 * 		if (is_valid_ether_addr(addr)) {
 * 			eth_hw_addr_set(bp->dev, addr);
 * 			return;
 * 		}
 * 	}
 * 
 * 	dev_info(&bp->pdev->dev, "invalid hw address, using random\n");
 * 	eth_hw_addr_random(bp->dev);
 * }
 * 
 * static int macb_mdio_wait_for_idle(struct macb *bp)
 * {
 * 	u32 val;
 * 
 * 	return readx_poll_timeout(MACB_READ_NSR, bp, val, val & MACB_BIT(IDLE),
 * 				  1, MACB_MDIO_TIMEOUT);
 * }
 */
static int macb_mdio_read_c22(struct mii_bus *bus, int mii_id, int regnum)
{
	struct macb *bp = bus->priv;
	int status;

	status = pm_runtime_resume_and_get(&bp->pdev->dev);
	if (status < 0)
		goto mdio_pm_exit;

	status = macb_mdio_wait_for_idle(bp);
	if (status < 0)
		goto mdio_read_exit;

	macb_writel(bp, MAN, (MACB_BF(SOF, MACB_MAN_C22_SOF)
			      | MACB_BF(RW, MACB_MAN_C22_READ)
			      | MACB_BF(PHYA, mii_id)
			      | MACB_BF(REGA, regnum)
			      | MACB_BF(CODE, MACB_MAN_C22_CODE)));

	status = macb_mdio_wait_for_idle(bp);
	if (status < 0)
		goto mdio_read_exit;

	status = MACB_BFEXT(DATA, macb_readl(bp, MAN));

mdio_read_exit:
	pm_runtime_put_autosuspend(&bp->pdev->dev);
mdio_pm_exit:
	return status;
}
/* Context-After
 * static int macb_mdio_read_c45(struct mii_bus *bus, int mii_id, int devad,
 * 			      int regnum)
 * {
 * 	struct macb *bp = bus->priv;
 * 	int status;
 * 
 * 	status = pm_runtime_get_sync(&bp->pdev->dev);
 * 	if (status < 0) {
 * 		pm_runtime_put_noidle(&bp->pdev->dev);
 * 		goto mdio_pm_exit;
 * 	}
 * 
 * 	status = macb_mdio_wait_for_idle(bp);
 * 	if (status < 0)
 * 		goto mdio_read_exit;
 * 
 * 	macb_writel(bp, MAN, (MACB_BF(SOF, MACB_MAN_C45_SOF)
 * 			      | MACB_BF(RW, MACB_MAN_C45_ADDR)
 * 			      | MACB_BF(PHYA, mii_id)
 */
