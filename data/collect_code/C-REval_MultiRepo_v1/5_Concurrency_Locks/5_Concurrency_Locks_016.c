/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_016 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 38 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/init.h>
 * #include <linux/kernel.h>
 * #include <linux/module.h>
 * #include <linux/pci.h>
 * #include <linux/io-64-nonatomic-lo-hi.h>
 * #include <linux/dmaengine.h>
 * #include <linux/irq.h>
 * #include <uapi/linux/idxd.h>
 * #include "../dmaengine.h"
 * #include "idxd.h"
 * #include "registers.h"
 */
/* Context-Before
 * 	union idxd_command_reg cmd;
 * 
 * 	if (idxd_device_is_halted(idxd)) {
 * 		dev_warn(&idxd->pdev->dev, "Device is HALTED!\n");
 * 		return -ENXIO;
 * 	}
 * 
 * 	memset(&cmd, 0, sizeof(cmd));
 * 	cmd.cmd = IDXD_CMD_RESET_DEVICE;
 * 	dev_dbg(dev, "%s: sending reset for init.\n", __func__);
 * 	spin_lock(&idxd->cmd_lock);
 * 	iowrite32(cmd.bits, idxd->reg_base + IDXD_CMD_OFFSET);
 * 
 * 	while (ioread32(idxd->reg_base + IDXD_CMDSTS_OFFSET) &
 * 	       IDXD_CMDSTS_ACTIVE)
 * 		cpu_relax();
 * 	spin_unlock(&idxd->cmd_lock);
 * 	return 0;
 * }
 */
static void idxd_cmd_exec(struct idxd_device *idxd, int cmd_code, u32 operand,
			  u32 *status)
{
	union idxd_command_reg cmd;
	DECLARE_COMPLETION_ONSTACK(done);
	u32 stat;
	unsigned long flags;

	if (idxd_device_is_halted(idxd)) {
		dev_warn(&idxd->pdev->dev, "Device is HALTED!\n");
		if (status)
			*status = IDXD_CMDSTS_HW_ERR;
		return;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd = cmd_code;
	cmd.operand = operand;
	cmd.int_req = 1;

	spin_lock_irqsave(&idxd->cmd_lock, flags);
	wait_event_lock_irq(idxd->cmd_waitq,
			    !test_bit(IDXD_FLAG_CMD_RUNNING, &idxd->flags),
			    idxd->cmd_lock);

	dev_dbg(&idxd->pdev->dev, "%s: sending cmd: %#x op: %#x\n",
		__func__, cmd_code, operand);

	idxd->cmd_status = 0;
	__set_bit(IDXD_FLAG_CMD_RUNNING, &idxd->flags);
	idxd->cmd_done = &done;
	iowrite32(cmd.bits, idxd->reg_base + IDXD_CMD_OFFSET);

	/*
	 * After command submitted, release lock and go to sleep until
	 * the command completes via interrupt.
	 */
	spin_unlock_irqrestore(&idxd->cmd_lock, flags);
	wait_for_completion(&done);
	stat = ioread32(idxd->reg_base + IDXD_CMDSTS_OFFSET);
	spin_lock(&idxd->cmd_lock);
	if (status)
		*status = stat;
	idxd->cmd_status = stat & GENMASK(7, 0);

	__clear_bit(IDXD_FLAG_CMD_RUNNING, &idxd->flags);
	/* Wake up other pending commands */
	wake_up(&idxd->cmd_waitq);
	spin_unlock(&idxd->cmd_lock);
}
/* Context-After
 * int idxd_device_enable(struct idxd_device *idxd)
 * {
 * 	struct device *dev = &idxd->pdev->dev;
 * 	u32 status;
 * 
 * 	if (idxd_is_enabled(idxd)) {
 * 		dev_dbg(dev, "Device already enabled\n");
 * 		return -ENXIO;
 * 	}
 * 
 * 	idxd_cmd_exec(idxd, IDXD_CMD_ENABLE_DEVICE, 0, &status);
 * 
 * 	/* If the command is successful or if the device was enabled */
 * 	if (status != IDXD_CMDSTS_SUCCESS &&
 * 	    status != IDXD_CMDSTS_ERR_DEV_ENABLED) {
 * 		dev_dbg(dev, "%s: err_code: %#x\n", __func__, status);
 * 		return -ENXIO;
 * 	}
 */
