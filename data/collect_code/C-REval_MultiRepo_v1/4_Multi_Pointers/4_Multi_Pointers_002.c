/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_002 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 10 */
/* NLOC: 37 */
/* Subsystem: sound */
/* Includes
 * #include <linux/acpi.h>
 * #include <linux/debugfs.h>
 * #include <linux/gpio/consumer.h>
 * #include <linux/module.h>
 * #include <linux/pm_runtime.h>
 * #include <linux/regmap.h>
 * #include <linux/slab.h>
 * #include <sound/core.h>
 * #include <sound/cs-amp-lib.h>
 * #include <sound/hda_codec.h>
 * #include <sound/tlv.h>
 * #include "cirrus_scodec.h"
 * #include "cs35l56_hda.h"
 * #include "hda_component.h"
 * #include "../generic.h"
 */
/* Context-Before
 * 	ctl_template.access = (SNDRV_CTL_ELEM_ACCESS_READWRITE | SNDRV_CTL_ELEM_ACCESS_TLV_READ);
 * 	ctl_template.tlv.p = cs35l56_hda_vol_tlv;
 * 	snprintf(name, sizeof(name), "%s Speaker Playback Volume", cs35l56->amp_name);
 * 	ctl_template.name = name;
 * 	cs35l56->volume_ctl = snd_ctl_new1(&ctl_template, cs35l56);
 * 	if (snd_ctl_add(cs35l56->codec->card, cs35l56->volume_ctl))
 * 		dev_err(cs35l56->base.dev, "Failed to add KControl: %s\n", ctl_template.name);
 * }
 * 
 * static void cs35l56_hda_remove_controls(struct cs35l56_hda *cs35l56)
 * {
 * 	int i;
 * 
 * 	for (i = ARRAY_SIZE(cs35l56->mixer_ctl) - 1; i >= 0; i--)
 * 		snd_ctl_remove(cs35l56->codec->card, cs35l56->mixer_ctl[i]);
 * 
 * 	snd_ctl_remove(cs35l56->codec->card, cs35l56->posture_ctl);
 * 	snd_ctl_remove(cs35l56->codec->card, cs35l56->volume_ctl);
 * }
 */
static int cs35l56_hda_request_firmware_file(struct cs35l56_hda *cs35l56,
					     const struct firmware **firmware, char **filename,
					     const char *base_name, const char *system_name,
					     const char *amp_name,
					     const char *filetype)
{
	char *s, c;
	int ret = 0;

	if (system_name && amp_name)
		*filename = kasprintf(GFP_KERNEL, "%s-%s-%s.%s", base_name,
				      system_name, amp_name, filetype);
	else if (system_name)
		*filename = kasprintf(GFP_KERNEL, "%s-%s.%s", base_name,
				      system_name, filetype);
	else
		*filename = kasprintf(GFP_KERNEL, "%s.%s", base_name, filetype);

	if (!*filename)
		return -ENOMEM;

	/*
	 * Make sure that filename is lower-case and any non alpha-numeric
	 * characters except full stop and forward slash are replaced with
	 * hyphens.
	 */
	s = *filename;
	while (*s) {
		c = *s;
		if (isalnum(c))
			*s = tolower(c);
		else if (c != '.' && c != '/')
			*s = '-';
		s++;
	}

	ret = firmware_request_nowarn(firmware, *filename, cs35l56->base.dev);
	if (ret) {
		dev_dbg(cs35l56->base.dev, "Failed to request '%s'\n", *filename);
		kfree(*filename);
		*filename = NULL;
		return ret;
	}

	dev_dbg(cs35l56->base.dev, "Found '%s'\n", *filename);

	return 0;
}
/* Context-After
 * static void cs35l56_hda_request_firmware_files(struct cs35l56_hda *cs35l56,
 * 					       unsigned int preloaded_fw_ver,
 * 					       const struct firmware **wmfw_firmware,
 * 					       char **wmfw_filename,
 * 					       const struct firmware **coeff_firmware,
 * 					       char **coeff_filename)
 * {
 * 	const char *system_name = cs35l56->system_name;
 * 	const char *amp_name = cs35l56->amp_name;
 * 	char base_name[37];
 * 	int ret;
 * 
 * 	if (preloaded_fw_ver) {
 * 		snprintf(base_name, sizeof(base_name),
 * 			 "cirrus/cs35l%02x-%02x%s-%06x-dsp1-misc",
 * 			 cs35l56->base.type,
 * 			 cs35l56->base.rev,
 * 			 cs35l56->base.secured ? "-s" : "",
 * 			 preloaded_fw_ver & 0xffffff);
 */
