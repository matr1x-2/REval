/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_039 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 14 */
/* NLOC: 54 */
/* Subsystem: sound */
/* Includes
 * #include <linux/input.h>
 * #include <linux/slab.h>
 * #include <linux/module.h>
 * #include <linux/ctype.h>
 * #include <linux/mm.h>
 * #include <linux/debugfs.h>
 * #include <sound/jack.h>
 * #include <sound/core.h>
 * #include <sound/control.h>
 */
/* Context-Before
 * 	snd_jack_kctl_add(jack, jack_kctl);
 * 	return 0;
 * }
 * EXPORT_SYMBOL(snd_jack_add_new_kctl);
 * 
 * /**
 *  * snd_jack_new - Create a new jack
 *  * @card:  the card instance
 *  * @id:    an identifying string for this jack
 *  * @type:  a bitmask of enum snd_jack_type values that can be detected by
 *  *         this jack
 *  * @jjack: Used to provide the allocated jack object to the caller.
 *  * @initial_kctl: if true, create a kcontrol and add it to the jack list.
 *  * @phantom_jack: Don't create a input device for phantom jacks.
 *  *
 *  * Creates a new jack object.
 *  *
 *  * Return: Zero if successful, or a negative error code on failure.
 *  * On success @jjack will be initialised.
 *  */
 */
int snd_jack_new(struct snd_card *card, const char *id, int type,
		 struct snd_jack **jjack, bool initial_kctl, bool phantom_jack)
{
	struct snd_jack *jack;
	struct snd_jack_kctl *jack_kctl = NULL;
	int err;
	static const struct snd_device_ops ops = {
		.dev_free = snd_jack_dev_free,
#ifdef CONFIG_SND_JACK_INPUT_DEV
		.dev_register = snd_jack_dev_register,
#endif /* CONFIG_SND_JACK_INPUT_DEV */
		.dev_disconnect = snd_jack_dev_disconnect,
	};

	if (initial_kctl) {
		jack_kctl = snd_jack_kctl_new(card, id, type);
		if (!jack_kctl)
			return -ENOMEM;
	}

	jack = kzalloc_obj(struct snd_jack);
	if (jack == NULL)
		return -ENOMEM;

	jack->id = kstrdup(id, GFP_KERNEL);
	if (jack->id == NULL) {
		kfree(jack);
		return -ENOMEM;
	}

#ifdef CONFIG_SND_JACK_INPUT_DEV
	mutex_init(&jack->input_dev_lock);

	/* don't create input device for phantom jack */
	if (!phantom_jack) {
		int i;

		jack->input_dev = input_allocate_device();
		if (jack->input_dev == NULL) {
			err = -ENOMEM;
			goto fail_input;
		}

		jack->input_dev->phys = "ALSA";

		jack->type = type;

		for (i = 0; i < SND_JACK_SWITCH_TYPES; i++)
			if (type & (1 << i))
				input_set_capability(jack->input_dev, EV_SW,
						     jack_switch_types[i]);

	}
#endif /* CONFIG_SND_JACK_INPUT_DEV */

	err = snd_device_new(card, SNDRV_DEV_JACK, jack, &ops);
	if (err < 0)
		goto fail_input;

	jack->card = card;
	INIT_LIST_HEAD(&jack->kctl_list);

	if (initial_kctl)
		snd_jack_kctl_add(jack, jack_kctl);

	*jjack = jack;

	return 0;

fail_input:
#ifdef CONFIG_SND_JACK_INPUT_DEV
	input_free_device(jack->input_dev);
#endif
	kfree(jack->id);
	kfree(jack);
	return err;
}
/* Context-After
 * EXPORT_SYMBOL(snd_jack_new);
 * 
 * #ifdef CONFIG_SND_JACK_INPUT_DEV
 * /**
 *  * snd_jack_set_key - Set a key mapping on a jack
 *  *
 *  * @jack:    The jack to configure
 *  * @type:    Jack report type for this key
 *  * @keytype: Input layer key type to be reported
 *  *
 *  * Map a SND_JACK_BTN_* button type to an input layer key, allowing
 *  * reporting of keys on accessories via the jack abstraction.  If no
 *  * mapping is provided but keys are enabled in the jack type then
 *  * BTN_n numeric buttons will be reported.
 *  *
 *  * If jacks are not reporting via the input API this call will have no
 *  * effect.
 *  *
 *  * Note that this is intended to be use by simple devices with small
 *  * numbers of keys that can be reported.  It is also possible to
 */
