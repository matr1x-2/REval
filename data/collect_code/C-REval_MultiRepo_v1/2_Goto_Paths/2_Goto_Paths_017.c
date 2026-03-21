/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_017 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 38 */
/* Subsystem: sound */
/* Includes
 * #include <linux/firmware.h>
 * #include <linux/module.h>
 * #include <linux/slab.h>
 * #include <linux/string.h>
 * #include <sound/hdaudio.h>
 * #include <sound/hdaudio_ext.h>
 * #include "avs.h"
 * #include "cldma.h"
 * #include "messages.h"
 * #include "registers.h"
 * #include "topology.h"
 */
/* Context-Before
 * 		return 0;
 * 	}
 * 
 * 	mod_ids = kcalloc(num_mods, sizeof(u16), GFP_KERNEL);
 * 	if (!mod_ids)
 * 		return -ENOMEM;
 * 
 * 	for (i = 0; i < num_mods; i++)
 * 		mod_ids[i] = mods[i].module_id;
 * 
 * 	ret = avs_ipc_unload_modules(adev, mod_ids, num_mods);
 * 	kfree(mod_ids);
 * 	if (ret)
 * 		return AVS_IPC_RET(ret);
 * 
 * 	return 0;
 * }
 * 
 * static int
 */
avs_hda_init_rom(struct avs_dev *adev, unsigned int dma_id, bool purge)
{
	const struct avs_spec *const spec = adev->spec;
	unsigned int corex_mask, reg;
	int ret;

	corex_mask = spec->core_init_mask & ~AVS_MAIN_CORE_MASK;

	ret = avs_dsp_op(adev, power, spec->core_init_mask, true);
	if (ret < 0)
		goto err;

	ret = avs_dsp_op(adev, reset, AVS_MAIN_CORE_MASK, false);
	if (ret < 0)
		goto err;

	reinit_completion(&adev->fw_ready);
	avs_dsp_op(adev, int_control, true);

	/* set boot config */
	ret = avs_ipc_set_boot_config(adev, dma_id, purge);
	if (ret) {
		ret = AVS_IPC_RET(ret);
		goto err;
	}

	/* await ROM init */
	ret = snd_hdac_adsp_readl_poll(adev, spec->hipc->sts_offset, reg,
				       (reg & 0xF) == AVS_ROM_INIT_DONE ||
				       (reg & 0xF) == APL_ROM_FW_ENTERED,
				       AVS_ROM_INIT_POLLING_US, APL_ROM_INIT_TIMEOUT_US);
	if (ret < 0) {
		dev_err(adev->dev, "rom init failed: %d, status: 0x%08x, lec: 0x%08x\n",
			ret, reg, snd_hdac_adsp_readl(adev, AVS_FW_REG_ERROR(adev)));
		goto err;
	}

	/* power down non-main cores */
	if (corex_mask) {
		ret = avs_dsp_op(adev, power, corex_mask, false);
		if (ret < 0)
			goto err;
	}

	return 0;

err:
	avs_dsp_core_disable(adev, spec->core_init_mask);
	return ret;
}
/* Context-After
 * static int avs_imr_load_basefw(struct avs_dev *adev)
 * {
 * 	int ret;
 * 
 * 	/* DMA id ignored when flashing from IMR as no transfer occurs. */
 * 	ret = avs_hda_init_rom(adev, 0, false);
 * 	if (ret < 0)
 * 		return ret;
 * 
 * 	ret = wait_for_completion_timeout(&adev->fw_ready,
 * 					  msecs_to_jiffies(AVS_FW_INIT_TIMEOUT_MS));
 * 	if (!ret) {
 * 		dev_err(adev->dev, "firmware ready timeout, status: 0x%08x, lec: 0x%08x\n",
 * 			snd_hdac_adsp_readl(adev, AVS_FW_REG_STATUS(adev)),
 * 			snd_hdac_adsp_readl(adev, AVS_FW_REG_ERROR(adev)));
 * 		avs_dsp_core_disable(adev, AVS_MAIN_CORE_MASK);
 * 		return -ETIMEDOUT;
 * 	}
 */
