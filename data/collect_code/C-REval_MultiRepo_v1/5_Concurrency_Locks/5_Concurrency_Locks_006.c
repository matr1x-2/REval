/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_006 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 27 */
/* Subsystem: sound */
/* Includes
 * #include <linux/acpi.h>
 * #include <linux/clk.h>
 * #include <linux/delay.h>
 * #include <linux/gpio.h>
 * #include <linux/init.h>
 * #include <linux/i2c.h>
 * #include <linux/module.h>
 * #include <linux/platform_device.h>
 * #include <linux/pm.h>
 * #include <linux/pm_runtime.h>
 * #include <linux/regmap.h>
 * #include <linux/slab.h>
 * #include <sound/core.h>
 * #include <sound/initval.h>
 * #include <sound/jack.h>
 * #include <sound/pcm.h>
 * #include <sound/pcm_params.h>
 * #include <sound/tlv.h>
 * #include <sound/soc.h>
 * #include <sound/soc-dapm.h>
 */
/* Context-Before
 * 	/* Switch MusicD3Live pin to GPIO */
 * 	regmap_write(cx2072x->regmap, CX2072X_DIGITAL_TEST1, 0);
 * 
 * 	snd_soc_dapm_mutex_lock(dapm);
 * 
 * 	snd_soc_dapm_force_enable_pin_unlocked(dapm, "PORTD");
 * 	snd_soc_dapm_force_enable_pin_unlocked(dapm, "Headset Bias");
 * 	snd_soc_dapm_force_enable_pin_unlocked(dapm, "PortD Mic Bias");
 * 
 * 	snd_soc_dapm_mutex_unlock(dapm);
 * }
 * 
 * static void cx2072x_disable_jack_detect(struct snd_soc_component *codec)
 * {
 * 	struct cx2072x_priv *cx2072x = snd_soc_component_get_drvdata(codec);
 * 
 * 	regmap_write(cx2072x->regmap, CX2072X_UM_INTERRUPT_CRTL_E, 0);
 * 	regmap_write(cx2072x->regmap, CX2072X_PORTA_UNSOLICITED_RESPONSE, 0);
 * }
 */
static int cx2072x_jack_status_check(void *data)
{
	struct snd_soc_component *codec = data;
	struct cx2072x_priv *cx2072x = snd_soc_component_get_drvdata(codec);
	unsigned int jack;
	unsigned int type = 0;
	int state = 0;

	mutex_lock(&cx2072x->lock);

	regmap_read(cx2072x->regmap, CX2072X_PORTA_PIN_SENSE, &jack);
	jack = jack >> 24;
	regmap_read(cx2072x->regmap, CX2072X_DIGITAL_TEST11, &type);

	if (jack == 0x80) {
		type = type >> 8;

		if (type & 0x8) {
			/* Apple headset */
			state |= SND_JACK_HEADSET;
			if (type & 0x2)
				state |= SND_JACK_BTN_0;
		} else {
			/*
			 * Nokia headset (type & 0x4) and
			 * regular Headphone
			 */
			state |= SND_JACK_HEADPHONE;
		}
	}

	/* clear interrupt */
	regmap_write(cx2072x->regmap, CX2072X_UM_INTERRUPT_CRTL_E, 0x12 << 24);

	mutex_unlock(&cx2072x->lock);

	dev_dbg(codec->dev, "CX2072X_HSDETECT type=0x%X,Jack state = %x\n",
		type, state);
	return state;
}
/* Context-After
 * static const struct snd_soc_jack_gpio cx2072x_jack_gpio = {
 * 	.name = "headset",
 * 	.report = SND_JACK_HEADSET | SND_JACK_BTN_0,
 * 	.debounce_time = 150,
 * 	.wake = true,
 * 	.jack_status_check = cx2072x_jack_status_check,
 * };
 * 
 * static int cx2072x_set_jack(struct snd_soc_component *codec,
 * 			    struct snd_soc_jack *jack, void *data)
 * {
 * 	struct cx2072x_priv *cx2072x = snd_soc_component_get_drvdata(codec);
 * 	int err;
 * 
 * 	if (!jack) {
 * 		cx2072x_disable_jack_detect(codec);
 * 		return 0;
 * 	}
 */
