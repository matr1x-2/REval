/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_050 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 141 */
/* NLOC: 152 */
/* Subsystem: sound */
/* Includes
 * #include <linux/module.h>
 * #include <linux/moduleparam.h>
 * #include <linux/init.h>
 * #include <linux/delay.h>
 * #include <linux/pm.h>
 * #include <linux/i2c.h>
 * #include <linux/platform_device.h>
 * #include <linux/spi/spi.h>
 * #include <linux/gpio/consumer.h>
 * #include <linux/acpi.h>
 * #include <linux/dmi.h>
 * #include <linux/regulator/consumer.h>
 * #include <sound/core.h>
 * #include <sound/pcm.h>
 * #include <sound/pcm_params.h>
 * #include <sound/jack.h>
 * #include <sound/soc.h>
 * #include <sound/soc-dapm.h>
 * #include <sound/initval.h>
 * #include <sound/tlv.h>
 */
/* Context-Before
 * 	case RT5645_IN1_CTRL2:
 * 	case RT5645_IN1_CTRL3:
 * 	case RT5645_A_JD_CTRL1:
 * 	case RT5645_ADC_EQ_CTRL1:
 * 	case RT5645_EQ_CTRL1:
 * 	case RT5645_ALC_CTRL_1:
 * 	case RT5645_IRQ_CTRL2:
 * 	case RT5645_IRQ_CTRL3:
 * 	case RT5645_INT_IRQ_ST:
 * 	case RT5645_IL_CMD:
 * 	case RT5650_4BTN_IL_CMD1:
 * 	case RT5645_VENDOR_ID:
 * 	case RT5645_VENDOR_ID1:
 * 	case RT5645_VENDOR_ID2:
 * 		return true;
 * 	default:
 * 		return false;
 * 	}
 * }
 */
static bool rt5645_readable_register(struct device *dev, unsigned int reg)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rt5645_ranges); i++) {
		if (reg >= rt5645_ranges[i].range_min &&
			reg <= rt5645_ranges[i].range_max) {
			return true;
		}
	}

	switch (reg) {
	case RT5645_RESET:
	case RT5645_SPK_VOL:
	case RT5645_HP_VOL:
	case RT5645_LOUT1:
	case RT5645_IN1_CTRL1:
	case RT5645_IN1_CTRL2:
	case RT5645_IN1_CTRL3:
	case RT5645_IN2_CTRL:
	case RT5645_INL1_INR1_VOL:
	case RT5645_SPK_FUNC_LIM:
	case RT5645_ADJ_HPF_CTRL:
	case RT5645_DAC1_DIG_VOL:
	case RT5645_DAC2_DIG_VOL:
	case RT5645_DAC_CTRL:
	case RT5645_STO1_ADC_DIG_VOL:
	case RT5645_MONO_ADC_DIG_VOL:
	case RT5645_ADC_BST_VOL1:
	case RT5645_ADC_BST_VOL2:
	case RT5645_STO1_ADC_MIXER:
	case RT5645_MONO_ADC_MIXER:
	case RT5645_AD_DA_MIXER:
	case RT5645_STO_DAC_MIXER:
	case RT5645_MONO_DAC_MIXER:
	case RT5645_DIG_MIXER:
	case RT5650_A_DAC_SOUR:
	case RT5645_DIG_INF1_DATA:
	case RT5645_PDM_OUT_CTRL:
	case RT5645_REC_L1_MIXER:
	case RT5645_REC_L2_MIXER:
	case RT5645_REC_R1_MIXER:
	case RT5645_REC_R2_MIXER:
	case RT5645_HPMIXL_CTRL:
	case RT5645_HPOMIXL_CTRL:
	case RT5645_HPMIXR_CTRL:
	case RT5645_HPOMIXR_CTRL:
	case RT5645_HPO_MIXER:
	case RT5645_SPK_L_MIXER:
	case RT5645_SPK_R_MIXER:
	case RT5645_SPO_MIXER:
	case RT5645_SPO_CLSD_RATIO:
	case RT5645_OUT_L1_MIXER:
	case RT5645_OUT_R1_MIXER:
	case RT5645_OUT_L_GAIN1:
	case RT5645_OUT_L_GAIN2:
	case RT5645_OUT_R_GAIN1:
	case RT5645_OUT_R_GAIN2:
	case RT5645_LOUT_MIXER:
	case RT5645_HAPTIC_CTRL1:
	case RT5645_HAPTIC_CTRL2:
	case RT5645_HAPTIC_CTRL3:
	case RT5645_HAPTIC_CTRL4:
	case RT5645_HAPTIC_CTRL5:
	case RT5645_HAPTIC_CTRL6:
	case RT5645_HAPTIC_CTRL7:
	case RT5645_HAPTIC_CTRL8:
	case RT5645_HAPTIC_CTRL9:
	case RT5645_HAPTIC_CTRL10:
	case RT5645_PWR_DIG1:
	case RT5645_PWR_DIG2:
	case RT5645_PWR_ANLG1:
	case RT5645_PWR_ANLG2:
	case RT5645_PWR_MIXER:
	case RT5645_PWR_VOL:
	case RT5645_PRIV_INDEX:
	case RT5645_PRIV_DATA:
	case RT5645_I2S1_SDP:
	case RT5645_I2S2_SDP:
	case RT5645_ADDA_CLK1:
	case RT5645_ADDA_CLK2:
	case RT5645_DMIC_CTRL1:
	case RT5645_DMIC_CTRL2:
	case RT5645_TDM_CTRL_1:
	case RT5645_TDM_CTRL_2:
	case RT5645_TDM_CTRL_3:
	case RT5650_TDM_CTRL_4:
	case RT5645_GLB_CLK:
	case RT5645_PLL_CTRL1:
	case RT5645_PLL_CTRL2:
	case RT5645_ASRC_1:
	case RT5645_ASRC_2:
	case RT5645_ASRC_3:
	case RT5645_ASRC_4:
	case RT5645_DEPOP_M1:
	case RT5645_DEPOP_M2:
	case RT5645_DEPOP_M3:
	case RT5645_CHARGE_PUMP:
	case RT5645_MICBIAS:
	case RT5645_A_JD_CTRL1:
	case RT5645_VAD_CTRL4:
	case RT5645_CLSD_OUT_CTRL:
	case RT5645_ADC_EQ_CTRL1:
	case RT5645_ADC_EQ_CTRL2:
	case RT5645_EQ_CTRL1:
	case RT5645_EQ_CTRL2:
	case RT5645_ALC_CTRL_1:
	case RT5645_ALC_CTRL_2:
	case RT5645_ALC_CTRL_3:
	case RT5645_ALC_CTRL_4:
	case RT5645_ALC_CTRL_5:
	case RT5645_JD_CTRL:
	case RT5645_IRQ_CTRL1:
	case RT5645_IRQ_CTRL2:
	case RT5645_IRQ_CTRL3:
	case RT5645_INT_IRQ_ST:
	case RT5645_GPIO_CTRL1:
	case RT5645_GPIO_CTRL2:
	case RT5645_GPIO_CTRL3:
	case RT5645_BASS_BACK:
	case RT5645_MP3_PLUS1:
	case RT5645_MP3_PLUS2:
	case RT5645_ADJ_HPF1:
	case RT5645_ADJ_HPF2:
	case RT5645_HP_CALIB_AMP_DET:
	case RT5645_SV_ZCD1:
	case RT5645_SV_ZCD2:
	case RT5645_IL_CMD:
	case RT5645_IL_CMD2:
	case RT5645_IL_CMD3:
	case RT5650_4BTN_IL_CMD1:
	case RT5650_4BTN_IL_CMD2:
	case RT5645_DRC1_HL_CTRL1:
	case RT5645_DRC2_HL_CTRL1:
	case RT5645_ADC_MONO_HP_CTRL1:
	case RT5645_ADC_MONO_HP_CTRL2:
	case RT5645_DRC2_CTRL1:
	case RT5645_DRC2_CTRL2:
	case RT5645_DRC2_CTRL3:
	case RT5645_DRC2_CTRL4:
	case RT5645_DRC2_CTRL5:
	case RT5645_JD_CTRL3:
	case RT5645_JD_CTRL4:
	case RT5645_GEN_CTRL1:
	case RT5645_GEN_CTRL2:
	case RT5645_GEN_CTRL3:
	case RT5645_VENDOR_ID:
	case RT5645_VENDOR_ID1:
	case RT5645_VENDOR_ID2:
		return true;
	default:
		return false;
	}
}
/* Context-After
 * static const DECLARE_TLV_DB_SCALE(out_vol_tlv, -4650, 150, 0);
 * static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -6525, 75, 0);
 * static const DECLARE_TLV_DB_SCALE(in_vol_tlv, -3450, 150, 0);
 * static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -1725, 75, 0);
 * static const DECLARE_TLV_DB_SCALE(adc_bst_tlv, 0, 1200, 0);
 * 
 * /* {0, +20, +24, +30, +35, +40, +44, +50, +52} dB */
 * static const DECLARE_TLV_DB_RANGE(bst_tlv,
 * 	0, 0, TLV_DB_SCALE_ITEM(0, 0, 0),
 * 	1, 1, TLV_DB_SCALE_ITEM(2000, 0, 0),
 * 	2, 2, TLV_DB_SCALE_ITEM(2400, 0, 0),
 * 	3, 5, TLV_DB_SCALE_ITEM(3000, 500, 0),
 * 	6, 6, TLV_DB_SCALE_ITEM(4400, 0, 0),
 * 	7, 7, TLV_DB_SCALE_ITEM(5000, 0, 0),
 * 	8, 8, TLV_DB_SCALE_ITEM(5200, 0, 0)
 * );
 * 
 * /* {-6, -4.5, -3, -1.5, 0, 0.82, 1.58, 2.28} dB */
 * static const DECLARE_TLV_DB_RANGE(spk_clsd_tlv,
 */
