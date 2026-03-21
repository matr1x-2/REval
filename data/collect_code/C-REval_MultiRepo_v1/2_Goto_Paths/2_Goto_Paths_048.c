/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_048 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 13 */
/* NLOC: 61 */
/* Subsystem: sound */
/* Includes
 * #include <linux/module.h>
 * #include <linux/delay.h>
 * #include <linux/init.h>
 * #include <linux/int_log.h>
 * #include <linux/i2c.h>
 * #include <linux/regmap.h>
 * #include <linux/slab.h>
 * #include <linux/clk.h>
 * #include <linux/acpi.h>
 * #include <linux/math64.h>
 * #include <linux/semaphore.h>
 * #include <sound/initval.h>
 * #include <sound/tlv.h>
 * #include <sound/core.h>
 * #include <sound/pcm.h>
 * #include <sound/pcm_params.h>
 * #include <sound/soc.h>
 * #include <sound/jack.h>
 * #include "nau8825.h"
 */
/* Context-Before
 * 		return &osr_adc_sel[osr];
 * 	}
 * }
 * 
 * static int nau8825_dai_startup(struct snd_pcm_substream *substream,
 * 			       struct snd_soc_dai *dai)
 * {
 * 	struct snd_soc_component *component = dai->component;
 * 	struct nau8825 *nau8825 = snd_soc_component_get_drvdata(component);
 * 	const struct nau8825_osr_attr *osr;
 * 
 * 	osr = nau8825_get_osr(nau8825, substream->stream);
 * 	if (!osr || !osr->osr)
 * 		return -EINVAL;
 * 
 * 	return snd_pcm_hw_constraint_minmax(substream->runtime,
 * 					    SNDRV_PCM_HW_PARAM_RATE,
 * 					    0, CLK_DA_AD_MAX / osr->osr);
 * }
 */
static int nau8825_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct nau8825 *nau8825 = snd_soc_component_get_drvdata(component);
	unsigned int val_len = 0, ctrl_val, bclk_fs, bclk_div;
	const struct nau8825_osr_attr *osr;
	int err = -EINVAL;

	nau8825_sema_acquire(nau8825, 3 * HZ);

	/* CLK_DAC or CLK_ADC = OSR * FS
	 * DAC or ADC clock frequency is defined as Over Sampling Rate (OSR)
	 * multiplied by the audio sample rate (Fs). Note that the OSR and Fs
	 * values must be selected such that the maximum frequency is less
	 * than 6.144 MHz.
	 */
	osr = nau8825_get_osr(nau8825, substream->stream);
	if (!osr || !osr->osr)
		goto error;
	if (params_rate(params) * osr->osr > CLK_DA_AD_MAX)
		goto error;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		regmap_update_bits(nau8825->regmap, NAU8825_REG_CLK_DIVIDER,
			NAU8825_CLK_DAC_SRC_MASK,
			osr->clk_src << NAU8825_CLK_DAC_SRC_SFT);
	else
		regmap_update_bits(nau8825->regmap, NAU8825_REG_CLK_DIVIDER,
			NAU8825_CLK_ADC_SRC_MASK,
			osr->clk_src << NAU8825_CLK_ADC_SRC_SFT);

	/* make BCLK and LRC divde configuration if the codec as master. */
	regmap_read(nau8825->regmap, NAU8825_REG_I2S_PCM_CTRL2, &ctrl_val);
	if (ctrl_val & NAU8825_I2S_MS_MASTER) {
		/* get the bclk and fs ratio */
		bclk_fs = snd_soc_params_to_bclk(params) / params_rate(params);
		if (bclk_fs <= 32)
			bclk_div = 2;
		else if (bclk_fs <= 64)
			bclk_div = 1;
		else if (bclk_fs <= 128)
			bclk_div = 0;
		else
			goto error;
		regmap_update_bits(nau8825->regmap, NAU8825_REG_I2S_PCM_CTRL2,
			NAU8825_I2S_LRC_DIV_MASK | NAU8825_I2S_BLK_DIV_MASK,
			((bclk_div + 1) << NAU8825_I2S_LRC_DIV_SFT) | bclk_div);
	}

	switch (params_width(params)) {
	case 16:
		val_len |= NAU8825_I2S_DL_16;
		break;
	case 20:
		val_len |= NAU8825_I2S_DL_20;
		break;
	case 24:
		val_len |= NAU8825_I2S_DL_24;
		break;
	case 32:
		val_len |= NAU8825_I2S_DL_32;
		break;
	default:
		goto error;
	}

	regmap_update_bits(nau8825->regmap, NAU8825_REG_I2S_PCM_CTRL1,
		NAU8825_I2S_DL_MASK, val_len);
	err = 0;

 error:
	/* Release the semaphore. */
	nau8825_sema_release(nau8825);

	return err;
}
/* Context-After
 * static int nau8825_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
 * {
 * 	struct snd_soc_component *component = codec_dai->component;
 * 	struct nau8825 *nau8825 = snd_soc_component_get_drvdata(component);
 * 	unsigned int ctrl1_val = 0, ctrl2_val = 0;
 * 
 * 	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
 * 	case SND_SOC_DAIFMT_CBP_CFP:
 * 		ctrl2_val |= NAU8825_I2S_MS_MASTER;
 * 		break;
 * 	case SND_SOC_DAIFMT_CBC_CFC:
 * 		break;
 * 	default:
 * 		return -EINVAL;
 * 	}
 * 
 * 	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
 * 	case SND_SOC_DAIFMT_NB_NF:
 * 		break;
 */
