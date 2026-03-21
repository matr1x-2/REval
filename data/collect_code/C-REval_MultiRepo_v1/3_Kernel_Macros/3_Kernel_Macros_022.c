/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_022 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 28 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/export.h>
 * #include <linux/interrupt.h>
 * #include <linux/irq.h>
 * #include <linux/kthread.h>
 * #include <linux/mfd/twl.h>
 * #include <linux/platform_device.h>
 * #include <linux/suspend.h>
 * #include <linux/of.h>
 * #include <linux/irqdomain.h>
 * #include <linux/of_device.h>
 * #include "twl-core.h"
 */
/* Context-Before
 * 	CHARGERFAULT_INTR_OFFSET,	/* Bit 21	EXT_CHRG	*/
 * 	CHARGERFAULT_INTR_OFFSET,	/* Bit 22	INT_CHRG	*/
 * 	RSV_INTR_OFFSET,	/* Bit 23	Reserved		*/
 * };
 * 
 * /*----------------------------------------------------------------------*/
 * 
 * struct twl6030_irq {
 * 	unsigned int		irq_base;
 * 	int			twl_irq;
 * 	bool			irq_wake_enabled;
 * 	atomic_t		wakeirqs;
 * 	struct notifier_block	pm_nb;
 * 	struct irq_chip		irq_chip;
 * 	struct irq_domain	*irq_domain;
 * 	const int		*irq_mapping_tbl;
 * };
 * 
 * static struct twl6030_irq *twl6030_irq;
 */
static int twl6030_irq_pm_notifier(struct notifier_block *notifier,
				   unsigned long pm_event, void *unused)
{
	int chained_wakeups;
	struct twl6030_irq *pdata = container_of(notifier, struct twl6030_irq,
						  pm_nb);

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		chained_wakeups = atomic_read(&pdata->wakeirqs);

		if (chained_wakeups && !pdata->irq_wake_enabled) {
			if (enable_irq_wake(pdata->twl_irq))
				pr_err("twl6030 IRQ wake enable failed\n");
			else
				pdata->irq_wake_enabled = true;
		} else if (!chained_wakeups && pdata->irq_wake_enabled) {
			disable_irq_wake(pdata->twl_irq);
			pdata->irq_wake_enabled = false;
		}

		disable_irq(pdata->twl_irq);
		break;

	case PM_POST_SUSPEND:
		enable_irq(pdata->twl_irq);
		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}
/* Context-After
 * /*
 * * Threaded irq handler for the twl6030 interrupt.
 * * We query the interrupt controller in the twl6030 to determine
 * * which module is generating the interrupt request and call
 * * handle_nested_irq for that module.
 * */
 * static irqreturn_t twl6030_irq_thread(int irq, void *data)
 * {
 * 	int i, ret;
 * 	union {
 * 		u8 bytes[4];
 * 		__le32 int_sts;
 * 	} sts;
 * 	u32 int_sts; /* sts.int_sts converted to CPU endianness */
 * 	struct twl6030_irq *pdata = data;
 * 
 * 	/* read INT_STS_A, B and C in one shot using a burst read */
 * 	ret = twl_i2c_read(TWL_MODULE_PIH, sts.bytes, REG_INT_STS_A, 3);
 * 	if (ret) {
 */
