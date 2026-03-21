/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_003 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 37 */
/* Subsystem: sound */
/* Includes
 * #include <linux/init.h>
 * #include <linux/jiffies.h>
 * #include <linux/slab.h>
 * #include <linux/time.h>
 * #include <linux/wait.h>
 * #include <linux/module.h>
 * #include <linux/platform_device.h>
 * #include <linux/firmware.h>
 * #include <linux/timer.h>
 * #include <linux/delay.h>
 * #include <linux/workqueue.h>
 * #include <linux/io.h>
 * #include <sound/core.h>
 * #include <sound/control.h>
 * #include <sound/pcm.h>
 * #include <sound/initval.h>
 * #include <sound/info.h>
 * #include <asm/dma.h>
 * #include <mach/sysasic.h>
 * #include "aica.h"
 */
/* Context-Before
 * 	    (dreamcastcard->card,
 * 	     snd_ctl_new1(&snd_aica_pcmvolume_control, dreamcastcard));
 * 	if (unlikely(err < 0))
 * 		return err;
 * 	err = snd_ctl_add
 * 	    (dreamcastcard->card,
 * 	     snd_ctl_new1(&snd_aica_pcmswitch_control, dreamcastcard));
 * 	if (unlikely(err < 0))
 * 		return err;
 * 	return 0;
 * }
 * 
 * static void snd_aica_remove(struct platform_device *devptr)
 * {
 * 	struct snd_card_aica *dreamcastcard;
 * 	dreamcastcard = platform_get_drvdata(devptr);
 * 	snd_card_free(dreamcastcard->card);
 * 	kfree(dreamcastcard);
 * }
 */
static int snd_aica_probe(struct platform_device *devptr)
{
	int err;
	struct snd_card_aica *dreamcastcard;
	dreamcastcard = kzalloc_obj(struct snd_card_aica);
	if (unlikely(!dreamcastcard))
		return -ENOMEM;
	err = snd_card_new(&devptr->dev, index, SND_AICA_DRIVER,
			   THIS_MODULE, 0, &dreamcastcard->card);
	if (unlikely(err < 0)) {
		kfree(dreamcastcard);
		return err;
	}
	strscpy(dreamcastcard->card->driver, "snd_aica");
	strscpy(dreamcastcard->card->shortname, SND_AICA_DRIVER);
	strscpy(dreamcastcard->card->longname,
	       "Yamaha AICA Super Intelligent Sound Processor for SEGA Dreamcast");
	/* Prepare to use the queue */
	INIT_WORK(&(dreamcastcard->spu_dma_work), run_spu_dma);
	timer_setup(&dreamcastcard->timer, aica_period_elapsed, 0);
	/* Load the PCM 'chip' */
	err = snd_aicapcmchip(dreamcastcard, 0);
	if (unlikely(err < 0))
		goto freedreamcast;
	/* Add basic controls */
	err = add_aicamixer_controls(dreamcastcard);
	if (unlikely(err < 0))
		goto freedreamcast;
	/* Register the card with ALSA subsystem */
	err = snd_card_register(dreamcastcard->card);
	if (unlikely(err < 0))
		goto freedreamcast;
	platform_set_drvdata(devptr, dreamcastcard);
	dev_info(&devptr->dev,
		 "ALSA Driver for Yamaha AICA Super Intelligent Sound Processor\n");
	return 0;
      freedreamcast:
	snd_card_free(dreamcastcard->card);
	kfree(dreamcastcard);
	return err;
}
/* Context-After
 * static struct platform_driver snd_aica_driver = {
 * 	.probe = snd_aica_probe,
 * 	.remove = snd_aica_remove,
 * 	.driver = {
 * 		.name = SND_AICA_DRIVER,
 * 	},
 * };
 * 
 * static int __init aica_init(void)
 * {
 * 	int err;
 * 	err = platform_driver_register(&snd_aica_driver);
 * 	if (unlikely(err < 0))
 * 		return err;
 * 	pd = platform_device_register_simple(SND_AICA_DRIVER, -1,
 * 					     aica_memory_space, 2);
 * 	if (IS_ERR(pd)) {
 * 		platform_driver_unregister(&snd_aica_driver);
 * 		return PTR_ERR(pd);
 */
