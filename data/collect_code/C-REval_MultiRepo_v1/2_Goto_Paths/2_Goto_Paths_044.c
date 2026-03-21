/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_044 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 13 */
/* NLOC: 46 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/slab.h>
 * #include <linux/export.h>
 * #include <linux/types.h>
 * #include <linux/scatterlist.h>
 * #include <linux/mmc/host.h>
 * #include <linux/mmc/card.h>
 * #include <linux/mmc/mmc.h>
 * #include "core.h"
 * #include "card.h"
 * #include "host.h"
 * #include "mmc_ops.h"
 */
/* Context-Before
 * 	cmd->busy_timeout = timeout_ms;
 * 	return true;
 * }
 * EXPORT_SYMBOL_GPL(mmc_prepare_busy_cmd);
 * 
 * /**
 *  *	__mmc_switch - modify EXT_CSD register
 *  *	@card: the MMC card associated with the data transfer
 *  *	@set: cmd set values
 *  *	@index: EXT_CSD register index
 *  *	@value: value to program into EXT_CSD register
 *  *	@timeout_ms: timeout (ms) for operation performed by register write,
 *  *                   timeout of zero implies maximum possible timeout
 *  *	@timing: new timing to change to
 *  *	@send_status: send status cmd to poll for busy
 *  *	@retry_crc_err: retry when CRC errors when polling with CMD13 for busy
 *  *	@retries: number of retries
 *  *
 *  *	Modifies the EXT_CSD register for selected card.
 *  */
 */
int __mmc_switch(struct mmc_card *card, u8 set, u8 index, u8 value,
		unsigned int timeout_ms, unsigned char timing,
		bool send_status, bool retry_crc_err, unsigned int retries)
{
	struct mmc_host *host = card->host;
	int err;
	struct mmc_command cmd = {};
	bool use_r1b_resp;
	unsigned char old_timing = host->ios.timing;

	mmc_retune_hold(host);

	if (!timeout_ms) {
		pr_warn("%s: unspecified timeout for CMD6 - use generic\n",
			mmc_hostname(host));
		timeout_ms = card->ext_csd.generic_cmd6_time;
	}

	cmd.opcode = MMC_SWITCH;
	cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		  (index << 16) |
		  (value << 8) |
		  set;
	use_r1b_resp = mmc_prepare_busy_cmd(host, &cmd, timeout_ms);

	err = mmc_wait_for_cmd(host, &cmd, retries);
	if (err)
		goto out;

	/*If SPI or used HW busy detection above, then we don't need to poll. */
	if (((host->caps & MMC_CAP_WAIT_WHILE_BUSY) && use_r1b_resp) ||
		mmc_host_is_spi(host))
		goto out_tim;

	/*
	 * If the host doesn't support HW polling via the ->card_busy() ops and
	 * when it's not allowed to poll by using CMD13, then we need to rely on
	 * waiting the stated timeout to be sufficient.
	 */
	if (!send_status && !host->ops->card_busy) {
		mmc_delay(timeout_ms);
		goto out_tim;
	}

	/* Let's try to poll to find out when the command is completed. */
	err = mmc_poll_for_busy(card, timeout_ms, retry_crc_err, MMC_BUSY_CMD6);
	if (err)
		goto out;

out_tim:
	/* Switch to new timing before check switch status. */
	if (timing)
		mmc_set_timing(host, timing);

	if (send_status) {
		err = mmc_switch_status(card, true);
		if (err && timing)
			mmc_set_timing(host, old_timing);
	}
out:
	mmc_retune_release(host);

	return err;
}
/* Context-After
 * int mmc_switch(struct mmc_card *card, u8 set, u8 index, u8 value,
 * 		unsigned int timeout_ms)
 * {
 * 	return __mmc_switch(card, set, index, value, timeout_ms, 0,
 * 			    true, false, MMC_CMD_RETRIES);
 * }
 * EXPORT_SYMBOL_GPL(mmc_switch);
 * 
 * int mmc_send_tuning(struct mmc_host *host, u32 opcode, int *cmd_error)
 * {
 * 	struct mmc_request mrq = {};
 * 	struct mmc_command cmd = {};
 * 	struct mmc_data data = {};
 * 	struct scatterlist sg;
 * 	struct mmc_ios *ios = &host->ios;
 * 	const u8 *tuning_block_pattern;
 * 	int size, err = 0;
 * 	u8 *data_buf;
 */
