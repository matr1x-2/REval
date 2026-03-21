/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_042 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 65 */
/* NLOC: 154 */
/* Subsystem: sound */
/* Includes
 * #include <linux/init.h>
 * #include <linux/slab.h>
 * #include <linux/sched/signal.h>
 * #include <linux/time.h>
 * #include <linux/vmalloc.h>
 * #include <linux/module.h>
 * #include <linux/math64.h>
 * #include <linux/string.h>
 * #include <linux/compat.h>
 * #include <sound/core.h>
 * #include <sound/minors.h>
 * #include <sound/pcm.h>
 * #include <sound/pcm_params.h>
 * #include "pcm_plugin.h"
 * #include <sound/info.h>
 * #include <linux/soundcard.h>
 * #include <sound/initval.h>
 * #include <sound/mixer_oss.h>
 */
/* Context-Before
 * 	struct snd_pcm_oss_file *pcm_oss_file;
 * 
 * 	pcm_oss_file = file->private_data;
 * 	substream = pcm_oss_file->streams[SNDRV_PCM_STREAM_PLAYBACK];
 * 	if (substream == NULL)
 * 		substream = pcm_oss_file->streams[SNDRV_PCM_STREAM_CAPTURE];
 * 	if (snd_BUG_ON(!substream))
 * 		return -ENXIO;
 * 	pcm = substream->pcm;
 * 	if (!pcm->card->shutdown)
 * 		snd_pcm_oss_sync(pcm_oss_file);
 * 	mutex_lock(&pcm->open_mutex);
 * 	snd_pcm_oss_release_file(pcm_oss_file);
 * 	mutex_unlock(&pcm->open_mutex);
 * 	wake_up(&pcm->open_wait);
 * 	module_put(pcm->card->module);
 * 	snd_card_file_remove(pcm->card, file);
 * 	return 0;
 * }
 */
static long snd_pcm_oss_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct snd_pcm_oss_file *pcm_oss_file;
	int __user *p = (int __user *)arg;
	int res;

	pcm_oss_file = file->private_data;
	if (cmd == OSS_GETVERSION)
		return put_user(SNDRV_OSS_VERSION, p);
	if (cmd == OSS_ALSAEMULVER)
		return put_user(1, p);
#if IS_REACHABLE(CONFIG_SND_MIXER_OSS)
	if (((cmd >> 8) & 0xff) == 'M')	{	/* mixer ioctl - for OSS compatibility */
		struct snd_pcm_substream *substream;
		int idx;
		for (idx = 0; idx < 2; ++idx) {
			substream = pcm_oss_file->streams[idx];
			if (substream != NULL)
				break;
		}
		if (snd_BUG_ON(idx >= 2))
			return -ENXIO;
		return snd_mixer_oss_ioctl_card(substream->pcm->card, cmd, arg);
	}
#endif
	if (((cmd >> 8) & 0xff) != 'P')
		return -EINVAL;
#ifdef OSS_DEBUG
	pr_debug("pcm_oss: ioctl = 0x%x\n", cmd);
#endif
	switch (cmd) {
	case SNDCTL_DSP_RESET:
		return snd_pcm_oss_reset(pcm_oss_file);
	case SNDCTL_DSP_SYNC:
		return snd_pcm_oss_sync(pcm_oss_file);
	case SNDCTL_DSP_SPEED:
		if (get_user(res, p))
			return -EFAULT;
		res = snd_pcm_oss_set_rate(pcm_oss_file, res);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SOUND_PCM_READ_RATE:
		res = snd_pcm_oss_get_rate(pcm_oss_file);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SNDCTL_DSP_STEREO:
		if (get_user(res, p))
			return -EFAULT;
		res = res > 0 ? 2 : 1;
		res = snd_pcm_oss_set_channels(pcm_oss_file, res);
		if (res < 0)
			return res;
		return put_user(--res, p);
	case SNDCTL_DSP_GETBLKSIZE:
		res = snd_pcm_oss_get_block_size(pcm_oss_file);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SNDCTL_DSP_SETFMT:
		if (get_user(res, p))
			return -EFAULT;
		res = snd_pcm_oss_set_format(pcm_oss_file, res);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SOUND_PCM_READ_BITS:
		res = snd_pcm_oss_get_format(pcm_oss_file);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SNDCTL_DSP_CHANNELS:
		if (get_user(res, p))
			return -EFAULT;
		res = snd_pcm_oss_set_channels(pcm_oss_file, res);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SOUND_PCM_READ_CHANNELS:
		res = snd_pcm_oss_get_channels(pcm_oss_file);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SOUND_PCM_WRITE_FILTER:
	case SOUND_PCM_READ_FILTER:
		return -EIO;
	case SNDCTL_DSP_POST:
		return snd_pcm_oss_post(pcm_oss_file);
	case SNDCTL_DSP_SUBDIVIDE:
		if (get_user(res, p))
			return -EFAULT;
		res = snd_pcm_oss_set_subdivide(pcm_oss_file, res);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SNDCTL_DSP_SETFRAGMENT:
		if (get_user(res, p))
			return -EFAULT;
		return snd_pcm_oss_set_fragment(pcm_oss_file, res);
	case SNDCTL_DSP_GETFMTS:
		res = snd_pcm_oss_get_formats(pcm_oss_file);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SNDCTL_DSP_GETOSPACE:
	case SNDCTL_DSP_GETISPACE:
		return snd_pcm_oss_get_space(pcm_oss_file,
			cmd == SNDCTL_DSP_GETISPACE ?
				SNDRV_PCM_STREAM_CAPTURE : SNDRV_PCM_STREAM_PLAYBACK,
			(struct audio_buf_info __user *) arg);
	case SNDCTL_DSP_NONBLOCK:
		return snd_pcm_oss_nonblock(file);
	case SNDCTL_DSP_GETCAPS:
		res = snd_pcm_oss_get_caps(pcm_oss_file);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SNDCTL_DSP_GETTRIGGER:
		res = snd_pcm_oss_get_trigger(pcm_oss_file);
		if (res < 0)
			return res;
		return put_user(res, p);
	case SNDCTL_DSP_SETTRIGGER:
		if (get_user(res, p))
			return -EFAULT;
		return snd_pcm_oss_set_trigger(pcm_oss_file, res);
	case SNDCTL_DSP_GETIPTR:
	case SNDCTL_DSP_GETOPTR:
		return snd_pcm_oss_get_ptr(pcm_oss_file,
			cmd == SNDCTL_DSP_GETIPTR ?
				SNDRV_PCM_STREAM_CAPTURE : SNDRV_PCM_STREAM_PLAYBACK,
			(struct count_info __user *) arg);
	case SNDCTL_DSP_MAPINBUF:
	case SNDCTL_DSP_MAPOUTBUF:
		return snd_pcm_oss_get_mapbuf(pcm_oss_file,
			cmd == SNDCTL_DSP_MAPINBUF ?
				SNDRV_PCM_STREAM_CAPTURE : SNDRV_PCM_STREAM_PLAYBACK,
			(struct buffmem_desc __user *) arg);
	case SNDCTL_DSP_SETSYNCRO:
		/* stop DMA now.. */
		return 0;
	case SNDCTL_DSP_SETDUPLEX:
		if (snd_pcm_oss_get_caps(pcm_oss_file) & DSP_CAP_DUPLEX)
			return 0;
		return -EIO;
	case SNDCTL_DSP_GETODELAY:
		res = snd_pcm_oss_get_odelay(pcm_oss_file);
		if (res < 0) {
			/* it's for sure, some broken apps don't check for error codes */
			put_user(0, p);
			return res;
		}
		return put_user(res, p);
	case SNDCTL_DSP_PROFILE:
		return 0;	/* silently ignore */
	default:
		pr_debug("pcm_oss: unknown command = 0x%x\n", cmd);
	}
	return -EINVAL;
}
/* Context-After
 * #ifdef CONFIG_COMPAT
 * /* all compatible */
 * static long snd_pcm_oss_ioctl_compat(struct file *file, unsigned int cmd,
 * 				     unsigned long arg)
 * {
 * 	/*
 * 	 * Everything is compatbile except SNDCTL_DSP_MAPINBUF/SNDCTL_DSP_MAPOUTBUF,
 * 	 * which are not implemented for the native case either
 * 	 */
 * 	return snd_pcm_oss_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
 * }
 * #else
 * #define snd_pcm_oss_ioctl_compat	NULL
 * #endif
 * 
 * static ssize_t snd_pcm_oss_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
 * {
 * 	struct snd_pcm_oss_file *pcm_oss_file;
 * 	struct snd_pcm_substream *substream;
 */
