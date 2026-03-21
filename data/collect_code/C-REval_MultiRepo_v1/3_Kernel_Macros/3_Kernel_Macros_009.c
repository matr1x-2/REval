/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_009 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 48 */
/* Subsystem: sound */
/* Includes
 * #include <linux/delay.h>
 * #include <linux/i2c.h>
 * #include <linux/module.h>
 * #include <linux/of.h>
 * #include <linux/pm.h>
 * #include <linux/regmap.h>
 * #include <linux/slab.h>
 * #include <linux/acpi.h>
 * #include <linux/clk.h>
 * #include <sound/jack.h>
 * #include <sound/pcm.h>
 * #include <sound/pcm_params.h>
 * #include <sound/soc.h>
 * #include <sound/tlv.h>
 * #include <sound/max98090.h>
 * #include "max98090.h"
 */
/* Context-Before
 * 	 */
 * 
 * 	/* Toggle shutdown OFF then ON */
 * 	snd_soc_component_update_bits(component, M98090_REG_DEVICE_SHUTDOWN,
 * 			    M98090_SHDNN_MASK, 0);
 * 	snd_soc_component_update_bits(component, M98090_REG_DEVICE_SHUTDOWN,
 * 			    M98090_SHDNN_MASK, M98090_SHDNN_MASK);
 * 
 * 	for (i = 0; i < 10; ++i) {
 * 		/* Give PLL time to lock */
 * 		usleep_range(1000, 1200);
 * 
 * 		/* Check lock status */
 * 		pll = snd_soc_component_read(
 * 				component, M98090_REG_DEVICE_STATUS);
 * 		if (!(pll & M98090_ULK_MASK))
 * 			break;
 * 	}
 * }
 */
static void max98090_jack_work(struct work_struct *work)
{
	struct max98090_priv *max98090 = container_of(work,
		struct max98090_priv,
		jack_work.work);
	struct snd_soc_component *component = max98090->component;
	int status = 0;
	int reg;

	/* Read a second time */
	if (max98090->jack_state == M98090_JACK_STATE_NO_HEADSET) {

		/* Strong pull up allows mic detection */
		snd_soc_component_update_bits(component, M98090_REG_JACK_DETECT,
			M98090_JDWK_MASK, 0);

		msleep(50);

		snd_soc_component_read(component, M98090_REG_JACK_STATUS);

		/* Weak pull up allows only insertion detection */
		snd_soc_component_update_bits(component, M98090_REG_JACK_DETECT,
			M98090_JDWK_MASK, M98090_JDWK_MASK);
	}

	reg = snd_soc_component_read(component, M98090_REG_JACK_STATUS);

	switch (reg & (M98090_LSNS_MASK | M98090_JKSNS_MASK)) {
		case M98090_LSNS_MASK | M98090_JKSNS_MASK:
			dev_dbg(component->dev, "No Headset Detected\n");

			max98090->jack_state = M98090_JACK_STATE_NO_HEADSET;

			status |= 0;

			break;

		case 0:
			if (max98090->jack_state ==
				M98090_JACK_STATE_HEADSET) {

				dev_dbg(component->dev,
					"Headset Button Down Detected\n");

				/*
				 * max98090_headset_button_event(codec)
				 * could be defined, then called here.
				 */

				status |= SND_JACK_HEADSET;
				status |= SND_JACK_BTN_0;

				break;
			}

			/* Line is reported as Headphone */
			/* Nokia Headset is reported as Headphone */
			/* Mono Headphone is reported as Headphone */
			dev_dbg(component->dev, "Headphone Detected\n");

			max98090->jack_state = M98090_JACK_STATE_HEADPHONE;

			status |= SND_JACK_HEADPHONE;

			break;

		case M98090_JKSNS_MASK:
			dev_dbg(component->dev, "Headset Detected\n");

			max98090->jack_state = M98090_JACK_STATE_HEADSET;

			status |= SND_JACK_HEADSET;

			break;

		default:
			dev_dbg(component->dev, "Unrecognized Jack Status\n");
			break;
	}

	snd_soc_jack_report(max98090->jack, status,
			    SND_JACK_HEADSET | SND_JACK_BTN_0);
}
/* Context-After
 * static irqreturn_t max98090_interrupt(int irq, void *data)
 * {
 * 	struct max98090_priv *max98090 = data;
 * 	struct snd_soc_component *component = max98090->component;
 * 	int ret;
 * 	unsigned int mask;
 * 	unsigned int active;
 * 
 * 	/* Treat interrupt before codec is initialized as spurious */
 * 	if (component == NULL)
 * 		return IRQ_NONE;
 * 
 * 	dev_dbg(component->dev, "***** max98090_interrupt *****\n");
 * 
 * 	ret = regmap_read(max98090->regmap, M98090_REG_INTERRUPT_S, &mask);
 * 
 * 	if (ret != 0) {
 * 		dev_err(component->dev,
 * 			"failed to read M98090_REG_INTERRUPT_S: %d\n",
 */
