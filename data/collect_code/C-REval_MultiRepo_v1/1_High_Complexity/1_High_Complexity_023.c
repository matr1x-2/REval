/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_023 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 25 */
/* NLOC: 107 */
/* Subsystem: sound */
/* Includes
 * #include <linux/init.h>
 * #include <linux/slab.h>
 * #include <linux/export.h>
 * #include <linux/sort.h>
 * #include <linux/delay.h>
 * #include <linux/ctype.h>
 * #include <linux/string.h>
 * #include <linux/bitops.h>
 * #include <linux/module.h>
 * #include <linux/leds.h>
 * #include <sound/core.h>
 * #include <sound/jack.h>
 * #include <sound/tlv.h>
 * #include <sound/hda_codec.h>
 * #include "hda_local.h"
 * #include "hda_auto_parser.h"
 * #include "hda_jack.h"
 * #include "hda_beep.h"
 * #include "generic.h"
 */
/* Context-Before
 * /* find all available DACs of the codec */
 * static void fill_all_dac_nids(struct hda_codec *codec)
 * {
 * 	struct hda_gen_spec *spec = codec->spec;
 * 	hda_nid_t nid;
 * 
 * 	spec->num_all_dacs = 0;
 * 	memset(spec->all_dacs, 0, sizeof(spec->all_dacs));
 * 	for_each_hda_codec_node(nid, codec) {
 * 		if (get_wcaps_type(get_wcaps(codec, nid)) != AC_WID_AUD_OUT)
 * 			continue;
 * 		if (spec->num_all_dacs >= ARRAY_SIZE(spec->all_dacs)) {
 * 			codec_err(codec, "Too many DACs!\n");
 * 			break;
 * 		}
 * 		spec->all_dacs[spec->num_all_dacs++] = nid;
 * 	}
 * }
 */
static int parse_output_paths(struct hda_codec *codec)
{
	struct hda_gen_spec *spec = codec->spec;
	struct auto_pin_cfg *cfg = &spec->autocfg;
	unsigned int val;
	int best_badness = INT_MAX;
	int badness;
	bool fill_hardwired = true, fill_mio_first = true;
	bool best_wired = true, best_mio = true;
	bool hp_spk_swapped = false;
	struct auto_pin_cfg *best_cfg __free(kfree) =
		kmalloc_obj(*best_cfg);

	if (!best_cfg)
		return -ENOMEM;
	*best_cfg = *cfg;

	for (;;) {
		badness = fill_and_eval_dacs(codec, fill_hardwired,
					     fill_mio_first);
		if (badness < 0)
			return badness;
		debug_badness("==> lo_type=%d, wired=%d, mio=%d, badness=0x%x\n",
			      cfg->line_out_type, fill_hardwired, fill_mio_first,
			      badness);
		debug_show_configs(codec, cfg);
		if (badness < best_badness) {
			best_badness = badness;
			*best_cfg = *cfg;
			best_wired = fill_hardwired;
			best_mio = fill_mio_first;
		}
		if (!badness)
			break;
		fill_mio_first = !fill_mio_first;
		if (!fill_mio_first)
			continue;
		fill_hardwired = !fill_hardwired;
		if (!fill_hardwired)
			continue;
		if (hp_spk_swapped)
			break;
		hp_spk_swapped = true;
		if (cfg->speaker_outs > 0 &&
		    cfg->line_out_type == AUTO_PIN_HP_OUT) {
			cfg->hp_outs = cfg->line_outs;
			memcpy(cfg->hp_pins, cfg->line_out_pins,
			       sizeof(cfg->hp_pins));
			cfg->line_outs = cfg->speaker_outs;
			memcpy(cfg->line_out_pins, cfg->speaker_pins,
			       sizeof(cfg->speaker_pins));
			cfg->speaker_outs = 0;
			memset(cfg->speaker_pins, 0, sizeof(cfg->speaker_pins));
			cfg->line_out_type = AUTO_PIN_SPEAKER_OUT;
			fill_hardwired = true;
			continue;
		}
		if (cfg->hp_outs > 0 &&
		    cfg->line_out_type == AUTO_PIN_SPEAKER_OUT) {
			cfg->speaker_outs = cfg->line_outs;
			memcpy(cfg->speaker_pins, cfg->line_out_pins,
			       sizeof(cfg->speaker_pins));
			cfg->line_outs = cfg->hp_outs;
			memcpy(cfg->line_out_pins, cfg->hp_pins,
			       sizeof(cfg->hp_pins));
			cfg->hp_outs = 0;
			memset(cfg->hp_pins, 0, sizeof(cfg->hp_pins));
			cfg->line_out_type = AUTO_PIN_HP_OUT;
			fill_hardwired = true;
			continue;
		}
		break;
	}

	if (badness) {
		debug_badness("==> restoring best_cfg\n");
		*cfg = *best_cfg;
		fill_and_eval_dacs(codec, best_wired, best_mio);
	}
	debug_badness("==> Best config: lo_type=%d, wired=%d, mio=%d\n",
		      cfg->line_out_type, best_wired, best_mio);
	debug_show_configs(codec, cfg);

	if (cfg->line_out_pins[0]) {
		struct nid_path *path;
		path = snd_hda_get_path_from_idx(codec, spec->out_paths[0]);
		if (path)
			spec->vmaster_nid = look_for_out_vol_nid(codec, path);
		if (spec->vmaster_nid) {
			snd_hda_set_vmaster_tlv(codec, spec->vmaster_nid,
						HDA_OUTPUT, spec->vmaster_tlv);
			if (spec->dac_min_mute)
				spec->vmaster_tlv[SNDRV_CTL_TLVO_DB_SCALE_MUTE_AND_STEP] |= TLV_DB_SCALE_MUTE;
		}
	}

	/* set initial pinctl targets */
	if (spec->prefer_hp_amp || cfg->line_out_type == AUTO_PIN_HP_OUT)
		val = PIN_HP;
	else
		val = PIN_OUT;
	set_pin_targets(codec, cfg->line_outs, cfg->line_out_pins, val);
	if (cfg->line_out_type != AUTO_PIN_HP_OUT)
		set_pin_targets(codec, cfg->hp_outs, cfg->hp_pins, PIN_HP);
	if (cfg->line_out_type != AUTO_PIN_SPEAKER_OUT) {
		val = spec->prefer_hp_amp ? PIN_HP : PIN_OUT;
		set_pin_targets(codec, cfg->speaker_outs,
				cfg->speaker_pins, val);
	}

	/* clear indep_hp flag if not available */
	if (spec->indep_hp && !indep_hp_possible(codec))
		spec->indep_hp = 0;

	return 0;
}
/* Context-After
 * /* add playback controls from the parsed DAC table */
 * static int create_multi_out_ctls(struct hda_codec *codec,
 * 				 const struct auto_pin_cfg *cfg)
 * {
 * 	struct hda_gen_spec *spec = codec->spec;
 * 	int i, err, noutputs;
 * 
 * 	noutputs = cfg->line_outs;
 * 	if (spec->multi_ios > 0 && cfg->line_outs < 3)
 * 		noutputs += spec->multi_ios;
 * 
 * 	for (i = 0; i < noutputs; i++) {
 * 		const char *name;
 * 		int index;
 * 		struct nid_path *path;
 * 
 * 		path = snd_hda_get_path_from_idx(codec, spec->out_paths[i]);
 * 		if (!path)
 * 			continue;
 */
