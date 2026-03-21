/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_029 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 10 */
/* NLOC: 52 */
/* Subsystem: sound */
/* Includes
 * #include <linux/init.h>
 * #include <linux/module.h>
 * #include <linux/slab.h>
 * #include <linux/suspend.h>
 * #include <sound/core.h>
 * #include <sound/pcm.h>
 * #include <sound/initval.h>
 * #include <sound/soc.h>
 * #include <asm/mach-au1x00/au1000.h>
 * #include <asm/mach-au1x00/au1xxx_psc.h>
 * #include "psc.h"
 */
/* Context-Before
 * /* supported I2S direction */
 * #define AU1XPSC_I2S_DIR \
 * 	(SND_SOC_DAIDIR_PLAYBACK | SND_SOC_DAIDIR_CAPTURE)
 * 
 * #define AU1XPSC_I2S_RATES \
 * 	SNDRV_PCM_RATE_8000_192000
 * 
 * #define AU1XPSC_I2S_FMTS \
 * 	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
 * 
 * #define I2SSTAT_BUSY(stype)	\
 * 	((stype) == SNDRV_PCM_STREAM_PLAYBACK ? PSC_I2SSTAT_TB : PSC_I2SSTAT_RB)
 * #define I2SPCR_START(stype)	\
 * 	((stype) == SNDRV_PCM_STREAM_PLAYBACK ? PSC_I2SPCR_TS : PSC_I2SPCR_RS)
 * #define I2SPCR_STOP(stype)	\
 * 	((stype) == SNDRV_PCM_STREAM_PLAYBACK ? PSC_I2SPCR_TP : PSC_I2SPCR_RP)
 * #define I2SPCR_CLRFIFO(stype)	\
 * 	((stype) == SNDRV_PCM_STREAM_PLAYBACK ? PSC_I2SPCR_TC : PSC_I2SPCR_RC)
 */
static int au1xpsc_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
			       unsigned int fmt)
{
	struct au1xpsc_audio_data *pscdata = snd_soc_dai_get_drvdata(cpu_dai);
	unsigned long ct;
	int ret;

	ret = -EINVAL;

	ct = pscdata->cfg;

	ct &= ~(PSC_I2SCFG_XM | PSC_I2SCFG_MLJ);	/* left-justified */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		ct |= PSC_I2SCFG_XM;	/* enable I2S mode */
		break;
	case SND_SOC_DAIFMT_MSB:
		break;
	case SND_SOC_DAIFMT_LSB:
		ct |= PSC_I2SCFG_MLJ;	/* LSB (right-) justified */
		break;
	default:
		goto out;
	}

	ct &= ~(PSC_I2SCFG_BI | PSC_I2SCFG_WI);		/* IB-IF */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		ct |= PSC_I2SCFG_BI | PSC_I2SCFG_WI;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		ct |= PSC_I2SCFG_BI;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		ct |= PSC_I2SCFG_WI;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		break;
	default:
		goto out;
	}

	switch (fmt & SND_SOC_DAIFMT_CLOCK_PROVIDER_MASK) {
	case SND_SOC_DAIFMT_BC_FC:	/* CODEC provider */
		ct |= PSC_I2SCFG_MS;	/* PSC I2S consumer mode */
		break;
	case SND_SOC_DAIFMT_BP_FP:	/* CODEC consumer */
		ct &= ~PSC_I2SCFG_MS;	/* PSC I2S provider mode */
		break;
	default:
		goto out;
	}

	pscdata->cfg = ct;
	ret = 0;
out:
	return ret;
}
/* Context-After
 * static int au1xpsc_i2s_hw_params(struct snd_pcm_substream *substream,
 * 				 struct snd_pcm_hw_params *params,
 * 				 struct snd_soc_dai *dai)
 * {
 * 	struct au1xpsc_audio_data *pscdata = snd_soc_dai_get_drvdata(dai);
 * 
 * 	int cfgbits;
 * 	unsigned long stat;
 * 
 * 	/* check if the PSC is already streaming data */
 * 	stat = __raw_readl(I2S_STAT(pscdata));
 * 	if (stat & (PSC_I2SSTAT_TB | PSC_I2SSTAT_RB)) {
 * 		/* reject parameters not currently set up in hardware */
 * 		cfgbits = __raw_readl(I2S_CFG(pscdata));
 * 		if ((PSC_I2SCFG_GET_LEN(cfgbits) != params->msbits) ||
 * 		    (params_rate(params) != pscdata->rate))
 * 			return -EINVAL;
 * 	} else {
 * 		/* set sample bitdepth */
 */
