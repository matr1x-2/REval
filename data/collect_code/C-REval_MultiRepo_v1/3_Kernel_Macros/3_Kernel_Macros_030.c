/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_030 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 37 */
/* Subsystem: sound */
/* Includes
 * #include <linux/acpi.h>
 * #include <linux/clk.h>
 * #include <linux/delay.h>
 * #include <linux/dmi.h>
 * #include <linux/i2c.h>
 * #include <linux/init.h>
 * #include <linux/math64.h>
 * #include <linux/module.h>
 * #include <linux/regmap.h>
 * #include <linux/slab.h>
 * #include <sound/core.h>
 * #include <sound/initval.h>
 * #include <sound/jack.h>
 * #include <sound/pcm.h>
 * #include <sound/pcm_params.h>
 * #include <sound/soc.h>
 * #include <sound/tlv.h>
 * #include "nau8821.h"
 */
/* Context-Before
 * 		nau8821_configure_sysclk(nau8821, NAU8821_CLK_DIS, 0);
 * 
 * 	/* Recover to normal channel input */
 * 	regmap_update_bits(regmap, NAU8821_R2B_ADC_RATE,
 * 			NAU8821_ADC_R_SRC_EN, 0);
 * 	if (nau8821->key_enable) {
 * 		regmap_update_bits(regmap, NAU8821_R0F_INTERRUPT_MASK,
 * 			NAU8821_IRQ_KEY_RELEASE_EN |
 * 			NAU8821_IRQ_KEY_PRESS_EN,
 * 			NAU8821_IRQ_KEY_RELEASE_EN |
 * 			NAU8821_IRQ_KEY_PRESS_EN);
 * 		regmap_update_bits(regmap,
 * 			NAU8821_R12_INTERRUPT_DIS_CTRL,
 * 			NAU8821_IRQ_KEY_RELEASE_DIS |
 * 			NAU8821_IRQ_KEY_PRESS_DIS,
 * 			NAU8821_IRQ_KEY_RELEASE_DIS |
 * 			NAU8821_IRQ_KEY_PRESS_DIS);
 * 	}
 * }
 */
static void nau8821_jdet_work(struct work_struct *work)
{
	struct nau8821 *nau8821 =
		container_of(work, struct nau8821, jdet_work.work);
	struct snd_soc_dapm_context *dapm = nau8821->dapm;
	struct regmap *regmap = nau8821->regmap;
	int jack_status_reg, mic_detected, event = 0, event_mask = 0;

	regmap_read(regmap, NAU8821_R58_I2C_DEVICE_ID, &jack_status_reg);
	mic_detected = !(jack_status_reg & NAU8821_KEYDET);
	if (mic_detected) {
		dev_dbg(nau8821->dev, "Headset connected\n");
		event |= SND_JACK_HEADSET;

		/* 2kOhm Resistor from MICBIAS to MICGND1 */
		regmap_update_bits(regmap, NAU8821_R74_MIC_BIAS,
			NAU8821_MICBIAS_JKR2, NAU8821_MICBIAS_JKR2);
		/* Latch Right Channel Analog data
		 * input into the Right Channel Filter
		 */
		regmap_update_bits(regmap, NAU8821_R2B_ADC_RATE,
			NAU8821_ADC_R_SRC_EN, NAU8821_ADC_R_SRC_EN);
		if (nau8821->key_enable) {
			regmap_update_bits(regmap, NAU8821_R0F_INTERRUPT_MASK,
				NAU8821_IRQ_KEY_RELEASE_EN |
				NAU8821_IRQ_KEY_PRESS_EN, 0);
			regmap_update_bits(regmap,
				NAU8821_R12_INTERRUPT_DIS_CTRL,
				NAU8821_IRQ_KEY_RELEASE_DIS |
				NAU8821_IRQ_KEY_PRESS_DIS, 0);
		} else {
			snd_soc_dapm_disable_pin(dapm, "MICBIAS");
			snd_soc_dapm_sync(dapm);
		}
	} else {
		dev_dbg(nau8821->dev, "Headphone connected\n");
		event |= SND_JACK_HEADPHONE;
		snd_soc_dapm_disable_pin(dapm, "MICBIAS");
		snd_soc_dapm_sync(dapm);
	}

	event_mask |= SND_JACK_HEADSET;
	snd_soc_jack_report(nau8821->jack, event, event_mask);
}
/* Context-After
 * /* Enable interruptions with internal clock. */
 * static void nau8821_setup_inserted_irq(struct nau8821 *nau8821)
 * {
 * 	struct regmap *regmap = nau8821->regmap;
 * 
 * 	/* Disable & mask insertion IRQ */
 * 	regmap_update_bits(regmap, NAU8821_R12_INTERRUPT_DIS_CTRL,
 * 			   NAU8821_IRQ_INSERT_DIS, NAU8821_IRQ_INSERT_DIS);
 * 	regmap_update_bits(regmap, NAU8821_R0F_INTERRUPT_MASK,
 * 			   NAU8821_IRQ_INSERT_EN, NAU8821_IRQ_INSERT_EN);
 * 
 * 	/* Clear insert IRQ status */
 * 	nau8821_irq_status_clear(regmap, NAU8821_JACK_INSERT_DETECTED);
 * 
 * 	/* Enable internal VCO needed for interruptions */
 * 	if (snd_soc_dapm_get_bias_level(nau8821->dapm) < SND_SOC_BIAS_PREPARE)
 * 		nau8821_configure_sysclk(nau8821, NAU8821_CLK_INTERNAL, 0);
 * 
 * 	/* Chip needs one FSCLK cycle in order to generate interruptions,
 */
