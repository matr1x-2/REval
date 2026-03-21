/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_008 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 140 */
/* NLOC: 146 */
/* Subsystem: sound */
/* Includes
 * #include <linux/module.h>
 * #include <linux/init.h>
 * #include <linux/clk.h>
 * #include <linux/io.h>
 * #include <linux/platform_device.h>
 * #include <linux/pm_runtime.h>
 * #include <linux/regmap.h>
 * #include <sound/soc.h>
 * #include <sound/soc-dapm.h>
 * #include <sound/tlv.h>
 * #include <linux/of_clk.h>
 * #include <linux/clk-provider.h>
 * #include "lpass-macro-common.h"
 */
/* Context-Before
 * 	{ CDC_TX7_TX_PATH_SEC4, 0x20},
 * 	{ CDC_TX7_TX_PATH_SEC5, 0x00},
 * 	{ CDC_TX7_TX_PATH_SEC6, 0x00},
 * };
 * 
 * static bool tx_is_volatile_register(struct device *dev, unsigned int reg)
 * {
 * 	/* Update volatile list for tx/tx macros */
 * 	switch (reg) {
 * 	case CDC_TX_TOP_CSR_SWR_DMIC0_CTL:
 * 	case CDC_TX_TOP_CSR_SWR_DMIC1_CTL:
 * 	case CDC_TX_TOP_CSR_SWR_DMIC2_CTL:
 * 	case CDC_TX_TOP_CSR_SWR_DMIC3_CTL:
 * 	case CDC_TX_TOP_CSR_SWR_AMIC0_CTL:
 * 	case CDC_TX_TOP_CSR_SWR_AMIC1_CTL:
 * 		return true;
 * 	}
 * 	return false;
 * }
 */
static bool tx_is_rw_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CDC_TX_CLK_RST_CTRL_MCLK_CONTROL:
	case CDC_TX_CLK_RST_CTRL_FS_CNT_CONTROL:
	case CDC_TX_CLK_RST_CTRL_SWR_CONTROL:
	case CDC_TX_TOP_CSR_TOP_CFG0:
	case CDC_TX_TOP_CSR_ANC_CFG:
	case CDC_TX_TOP_CSR_SWR_CTRL:
	case CDC_TX_TOP_CSR_FREQ_MCLK:
	case CDC_TX_TOP_CSR_DEBUG_BUS:
	case CDC_TX_TOP_CSR_DEBUG_EN:
	case CDC_TX_TOP_CSR_TX_I2S_CTL:
	case CDC_TX_TOP_CSR_I2S_CLK:
	case CDC_TX_TOP_CSR_I2S_RESET:
	case CDC_TX_TOP_CSR_SWR_DMIC0_CTL:
	case CDC_TX_TOP_CSR_SWR_DMIC1_CTL:
	case CDC_TX_TOP_CSR_SWR_DMIC2_CTL:
	case CDC_TX_TOP_CSR_SWR_DMIC3_CTL:
	case CDC_TX_TOP_CSR_SWR_AMIC0_CTL:
	case CDC_TX_TOP_CSR_SWR_AMIC1_CTL:
	case CDC_TX_ANC0_CLK_RESET_CTL:
	case CDC_TX_ANC0_MODE_1_CTL:
	case CDC_TX_ANC0_MODE_2_CTL:
	case CDC_TX_ANC0_FF_SHIFT:
	case CDC_TX_ANC0_FB_SHIFT:
	case CDC_TX_ANC0_LPF_FF_A_CTL:
	case CDC_TX_ANC0_LPF_FF_B_CTL:
	case CDC_TX_ANC0_LPF_FB_CTL:
	case CDC_TX_ANC0_SMLPF_CTL:
	case CDC_TX_ANC0_DCFLT_SHIFT_CTL:
	case CDC_TX_ANC0_IIR_ADAPT_CTL:
	case CDC_TX_ANC0_IIR_COEFF_1_CTL:
	case CDC_TX_ANC0_IIR_COEFF_2_CTL:
	case CDC_TX_ANC0_FF_A_GAIN_CTL:
	case CDC_TX_ANC0_FF_B_GAIN_CTL:
	case CDC_TX_ANC0_FB_GAIN_CTL:
	case CDC_TX_INP_MUX_ADC_MUX0_CFG0:
	case CDC_TX_INP_MUX_ADC_MUX0_CFG1:
	case CDC_TX_INP_MUX_ADC_MUX1_CFG0:
	case CDC_TX_INP_MUX_ADC_MUX1_CFG1:
	case CDC_TX_INP_MUX_ADC_MUX2_CFG0:
	case CDC_TX_INP_MUX_ADC_MUX2_CFG1:
	case CDC_TX_INP_MUX_ADC_MUX3_CFG0:
	case CDC_TX_INP_MUX_ADC_MUX3_CFG1:
	case CDC_TX_INP_MUX_ADC_MUX4_CFG0:
	case CDC_TX_INP_MUX_ADC_MUX4_CFG1:
	case CDC_TX_INP_MUX_ADC_MUX5_CFG0:
	case CDC_TX_INP_MUX_ADC_MUX5_CFG1:
	case CDC_TX_INP_MUX_ADC_MUX6_CFG0:
	case CDC_TX_INP_MUX_ADC_MUX6_CFG1:
	case CDC_TX_INP_MUX_ADC_MUX7_CFG0:
	case CDC_TX_INP_MUX_ADC_MUX7_CFG1:
	case CDC_TX0_TX_PATH_CTL:
	case CDC_TX0_TX_PATH_CFG0:
	case CDC_TX0_TX_PATH_CFG1:
	case CDC_TX0_TX_VOL_CTL:
	case CDC_TX0_TX_PATH_SEC0:
	case CDC_TX0_TX_PATH_SEC1:
	case CDC_TX0_TX_PATH_SEC2:
	case CDC_TX0_TX_PATH_SEC3:
	case CDC_TX0_TX_PATH_SEC4:
	case CDC_TX0_TX_PATH_SEC5:
	case CDC_TX0_TX_PATH_SEC6:
	case CDC_TX0_TX_PATH_SEC7:
	case CDC_TX1_TX_PATH_CTL:
	case CDC_TX1_TX_PATH_CFG0:
	case CDC_TX1_TX_PATH_CFG1:
	case CDC_TX1_TX_VOL_CTL:
	case CDC_TX1_TX_PATH_SEC0:
	case CDC_TX1_TX_PATH_SEC1:
	case CDC_TX1_TX_PATH_SEC2:
	case CDC_TX1_TX_PATH_SEC3:
	case CDC_TX1_TX_PATH_SEC4:
	case CDC_TX1_TX_PATH_SEC5:
	case CDC_TX1_TX_PATH_SEC6:
	case CDC_TX2_TX_PATH_CTL:
	case CDC_TX2_TX_PATH_CFG0:
	case CDC_TX2_TX_PATH_CFG1:
	case CDC_TX2_TX_VOL_CTL:
	case CDC_TX2_TX_PATH_SEC0:
	case CDC_TX2_TX_PATH_SEC1:
	case CDC_TX2_TX_PATH_SEC2:
	case CDC_TX2_TX_PATH_SEC3:
	case CDC_TX2_TX_PATH_SEC4:
	case CDC_TX2_TX_PATH_SEC5:
	case CDC_TX2_TX_PATH_SEC6:
	case CDC_TX3_TX_PATH_CTL:
	case CDC_TX3_TX_PATH_CFG0:
	case CDC_TX3_TX_PATH_CFG1:
	case CDC_TX3_TX_VOL_CTL:
	case CDC_TX3_TX_PATH_SEC0:
	case CDC_TX3_TX_PATH_SEC1:
	case CDC_TX3_TX_PATH_SEC2:
	case CDC_TX3_TX_PATH_SEC3:
	case CDC_TX3_TX_PATH_SEC4:
	case CDC_TX3_TX_PATH_SEC5:
	case CDC_TX3_TX_PATH_SEC6:
	case CDC_TX4_TX_PATH_CTL:
	case CDC_TX4_TX_PATH_CFG0:
	case CDC_TX4_TX_PATH_CFG1:
	case CDC_TX4_TX_VOL_CTL:
	case CDC_TX4_TX_PATH_SEC0:
	case CDC_TX4_TX_PATH_SEC1:
	case CDC_TX4_TX_PATH_SEC2:
	case CDC_TX4_TX_PATH_SEC3:
	case CDC_TX4_TX_PATH_SEC4:
	case CDC_TX4_TX_PATH_SEC5:
	case CDC_TX4_TX_PATH_SEC6:
	case CDC_TX5_TX_PATH_CTL:
	case CDC_TX5_TX_PATH_CFG0:
	case CDC_TX5_TX_PATH_CFG1:
	case CDC_TX5_TX_VOL_CTL:
	case CDC_TX5_TX_PATH_SEC0:
	case CDC_TX5_TX_PATH_SEC1:
	case CDC_TX5_TX_PATH_SEC2:
	case CDC_TX5_TX_PATH_SEC3:
	case CDC_TX5_TX_PATH_SEC4:
	case CDC_TX5_TX_PATH_SEC5:
	case CDC_TX5_TX_PATH_SEC6:
	case CDC_TX6_TX_PATH_CTL:
	case CDC_TX6_TX_PATH_CFG0:
	case CDC_TX6_TX_PATH_CFG1:
	case CDC_TX6_TX_VOL_CTL:
	case CDC_TX6_TX_PATH_SEC0:
	case CDC_TX6_TX_PATH_SEC1:
	case CDC_TX6_TX_PATH_SEC2:
	case CDC_TX6_TX_PATH_SEC3:
	case CDC_TX6_TX_PATH_SEC4:
	case CDC_TX6_TX_PATH_SEC5:
	case CDC_TX6_TX_PATH_SEC6:
	case CDC_TX7_TX_PATH_CTL:
	case CDC_TX7_TX_PATH_CFG0:
	case CDC_TX7_TX_PATH_CFG1:
	case CDC_TX7_TX_VOL_CTL:
	case CDC_TX7_TX_PATH_SEC0:
	case CDC_TX7_TX_PATH_SEC1:
	case CDC_TX7_TX_PATH_SEC2:
	case CDC_TX7_TX_PATH_SEC3:
	case CDC_TX7_TX_PATH_SEC4:
	case CDC_TX7_TX_PATH_SEC5:
	case CDC_TX7_TX_PATH_SEC6:
		return true;
	}

	return false;
}
/* Context-After
 * static const struct regmap_config tx_regmap_config = {
 * 	.name = "tx_macro",
 * 	.reg_bits = 16,
 * 	.val_bits = 32,
 * 	.reg_stride = 4,
 * 	.cache_type = REGCACHE_FLAT,
 * 	.max_register = TX_MAX_OFFSET,
 * 	.reg_defaults = tx_defaults,
 * 	.num_reg_defaults = ARRAY_SIZE(tx_defaults),
 * 	.writeable_reg = tx_is_rw_register,
 * 	.volatile_reg = tx_is_volatile_register,
 * 	.readable_reg = tx_is_rw_register,
 * };
 * 
 * static int tx_macro_mclk_enable(struct tx_macro *tx,
 * 				bool mclk_enable)
 * {
 * 	struct regmap *regmap = tx->regmap;
 */
