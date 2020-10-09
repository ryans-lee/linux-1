// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2020, Maxim Integrated

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <sound/tlv.h>
#include "max98396.h"

static struct reg_default max98396_reg[] = {
	{MAX98396_R2000_SW_RESET, 0x00},
	{MAX98396_R2001_INT_RAW1, 0x00},
	{MAX98396_R2002_INT_RAW2, 0x00},
	{MAX98396_R2003_INT_RAW3, 0x00},
	{MAX98396_R2004_INT_RAW4, 0x00},
	{MAX98396_R2006_INT_STATE1, 0x00},
	{MAX98396_R2007_INT_STATE2, 0x00},
	{MAX98396_R2008_INT_STATE3, 0x00},
	{MAX98396_R2009_INT_STATE4, 0x00},
	{MAX98396_R200B_INT_FLAG1, 0x00},
	{MAX98396_R200C_INT_FLAG2, 0x00},
	{MAX98396_R200D_INT_FLAG3, 0x00},
	{MAX98396_R200E_INT_FLAG4, 0x00},
	{MAX98396_R2010_INT_EN1, 0x02},
	{MAX98396_R2011_INT_EN2, 0x00},
	{MAX98396_R2012_INT_EN3, 0x00},
	{MAX98396_R2013_INT_EN4, 0x00},
	{MAX98396_R2015_INT_FLAG_CLR1, 0x00},
	{MAX98396_R2016_INT_FLAG_CLR2, 0x00},
	{MAX98396_R2017_INT_FLAG_CLR3, 0x00},
	{MAX98396_R2018_INT_FLAG_CLR4, 0x00},
	{MAX98396_R201F_IRQ_CTRL, 0x00},
	{MAX98396_R2020_THERM_WARN_THRESH, 0x46},
	{MAX98396_R2021_THERM_WARN_THRESH2, 0x46},
	{MAX98396_R2022_THERM_SHDN_THRESH, 0x64},
	{MAX98396_R2023_THERM_HYSTERESIS, 0x02},
	{MAX98396_R2024_THERM_FOLDBACK_SET, 0xC5},
	{MAX98396_R2027_THERM_FOLDBACK_EN, 0x01},
	{MAX98396_R2030_NOISE_GATE_IDLE_MODE_CTRL, 0x32},
	{MAX98396_R2033_NOISE_GATE_IDLE_MODE_EN, 0x00},
	{MAX98396_R2038_CLK_MON_CTRL, 0x00},
	{MAX98396_R2039_DATA_MON_CTRL, 0x00},
	{MAX98396_R203F_ENABLE_CTRLS, 0x0F},
	{MAX98396_R2040_PIN_CFG, 0x55},
	{MAX98396_R2041_PCM_MODE_CFG, 0xC0},
	{MAX98396_R2042_PCM_CLK_SETUP, 0x04},
	{MAX98396_R2043_PCM_SR_SETUP, 0x88},
	{MAX98396_R2044_PCM_TX_CTRL_1, 0x00},
	{MAX98396_R2045_PCM_TX_CTRL_2, 0x00},
	{MAX98396_R2046_PCM_TX_CTRL_3, 0x00},
	{MAX98396_R2047_PCM_TX_CTRL_4, 0x00},
	{MAX98396_R2048_PCM_TX_CTRL_5, 0x00},
	{MAX98396_R2049_PCM_TX_CTRL_6, 0x00},
	{MAX98396_R204A_PCM_TX_CTRL_7, 0x00},
	{MAX98396_R204B_PCM_TX_CTRL_8, 0x00},
	{MAX98396_R204C_PCM_TX_HIZ_CTRL_1, 0xFF},
	{MAX98396_R204D_PCM_TX_HIZ_CTRL_2, 0xFF},
	{MAX98396_R204E_PCM_TX_HIZ_CTRL_3, 0xFF},
	{MAX98396_R204F_PCM_TX_HIZ_CTRL_4, 0xFF},
	{MAX98396_R2050_PCM_TX_HIZ_CTRL_5, 0xFF},
	{MAX98396_R2051_PCM_TX_HIZ_CTRL_6, 0xFF},
	{MAX98396_R2052_PCM_TX_HIZ_CTRL_7, 0xFF},
	{MAX98396_R2053_PCM_TX_HIZ_CTRL_8, 0xFF},
	{MAX98396_R2055_PCM_RX_SRC1, 0x00},
	{MAX98396_R2056_PCM_RX_SRC2, 0x00},
	{MAX98396_R2058_PCM_BYPASS_SRC, 0x00},
	{MAX98396_R205D_PCM_TX_SRC_EN, 0x00},
	{MAX98396_R205E_PCM_RX_EN, 0x00},
	{MAX98396_R205F_PCM_TX_EN, 0x00},
	{MAX98396_R2070_ICC_RX_EN_A, 0x00},
	{MAX98396_R2071_ICC_RX_EN_B, 0x00},
	{MAX98396_R2072_ICC_TX_CTRL, 0x00},
	{MAX98396_R207F_ICC_EN, 0x00},
	{MAX98396_R2083_TONE_GEN_DC_CFG, 0x04},
	{MAX98396_R2084_TONE_GEN_DC_LVL1, 0x00},
	{MAX98396_R2085_TONE_GEN_DC_LVL2, 0x00},
	{MAX98396_R2086_TONE_GEN_DC_LVL3, 0x00},
	{MAX98396_R208F_TONE_GEN_EN, 0x00},
	{MAX98396_R2090_AMP_VOL_CTRL, 0x00},
	{MAX98396_R2091_AMP_PATH_GAIN, 0x0B},
	{MAX98396_R2092_AMP_DSP_CFG, 0x23},
	{MAX98396_R2093_SSM_CFG, 0x0D},
	{MAX98396_R2094_SPK_CLS_DG_THRESH, 0x12},
	{MAX98396_R2095_SPK_CLS_DG_HDR, 0x17},
	{MAX98396_R2096_SPK_CLS_DG_HOLD_TIME, 0x17},
	{MAX98396_R2097_SPK_CLS_DG_DELAY, 0x00},
	{MAX98396_R2098_SPK_CLS_DG_MODE, 0x00},
	{MAX98396_R2099_SPK_CLS_DG_VBAT_LVL, 0x03},
	{MAX98396_R209A_SPK_EDGE_CTRL, 0x00},
	{MAX98396_R209C_SPK_EDGE_CTRL1, 0x0A},
	{MAX98396_R209D_SPK_EDGE_CTRL2, 0xAA},
	{MAX98396_R209E_AMP_CLIP_GAIN, 0x00},
	{MAX98396_R209F_BYPASS_PATH_CFG, 0x00},
	{MAX98396_R20A0_AMP_SUPPLY_CTL, 0x00},
	{MAX98396_R20AF_AMP_EN, 0x00},
	{MAX98396_R20B0_MEAS_ADC_SR, 0x30},
	{MAX98396_R20B1_MEAS_ADC_PVDD_CFG, 0x00},
	{MAX98396_R20B2_MEAS_ADC_VBAT_CFG, 0x00},
	{MAX98396_R20B3_MEAS_ADC_THERMAL_CFG, 0x00},
	{MAX98396_R20B4_ADC_READBACK_CTRL1, 0x00},
	{MAX98396_R20B5_ADC_READBACK_CTRL2, 0x00},
	{MAX98396_R20B6_ADC_PVDD_READBACK_MSB, 0x00},
	{MAX98396_R20B7_ADC_PVDD_READBACK_LSB, 0x00},
	{MAX98396_R20B8_ADC_VBAT_READBACK_MSB, 0x00},
	{MAX98396_R20B9_ADC_VBAT_READBACK_LSB, 0x00},
	{MAX98396_R20BA_TEMP_READBACK_MSB, 0x00},
	{MAX98396_R20BB_TEMP_READBACK_LSB, 0x00},
	{MAX98396_R20BC_LO_PVDD_READBACK_MSB, 0xFF},
	{MAX98396_R20BD_LO_PVDD_READBACK_LSB, 0x01},
	{MAX98396_R20BE_LO_VBAT_READBACK_MSB, 0xFF},
	{MAX98396_R20BF_LO_VBAT_READBACK_LSB, 0x01},
	{MAX98396_R20C7_MEAS_ADC_CFG, 0x00},
	{MAX98396_R20D0_DHT_CFG1, 0x00},
	{MAX98396_R20D1_LIMITER_CFG1, 0x08},
	{MAX98396_R20D2_LIMITER_CFG2, 0x00},
	{MAX98396_R20D3_DHT_CFG2, 0x14},
	{MAX98396_R20D4_DHT_CFG3, 0x02},
	{MAX98396_R20D5_DHT_CFG4, 0x04},
	{MAX98396_R20D6_DHT_HYSTERESIS_CFG, 0x07},
	{MAX98396_R20DF_DHT_EN, 0x00},
	{MAX98396_R20E0_IV_SENSE_PATH_CFG, 0x04},
	{MAX98396_R20E4_IV_SENSE_PATH_EN, 0x00},
	{MAX98396_R20E5_BPE_STATE, 0x00},
	{MAX98396_R20E6_BPE_L3_THRESH_MSB, 0x00},
	{MAX98396_R20E7_BPE_L3_THRESH_LSB, 0x00},
	{MAX98396_R20E8_BPE_L2_THRESH_MSB, 0x00},
	{MAX98396_R20E9_BPE_L2_THRESH_LSB, 0x00},
	{MAX98396_R20EA_BPE_L1_THRESH_MSB, 0x00},
	{MAX98396_R20EB_BPE_L1_THRESH_LSB, 0x00},
	{MAX98396_R20EC_BPE_L0_THRESH_MSB, 0x00},
	{MAX98396_R20ED_BPE_L0_THRESH_LSB, 0x00},
	{MAX98396_R20EE_BPE_L3_DWELL_HOLD_TIME, 0x00},
	{MAX98396_R20EF_BPE_L2_DWELL_HOLD_TIME, 0x00},
	{MAX98396_R20F0_BPE_L1_DWELL_HOLD_TIME, 0x00},
	{MAX98396_R20F1_BPE_L0_HOLD_TIME, 0x00},
	{MAX98396_R20F2_BPE_L3_ATTACK_REL_STEP, 0x00},
	{MAX98396_R20F3_BPE_L2_ATTACK_REL_STEP, 0x00},
	{MAX98396_R20F4_BPE_L1_ATTACK_REL_STEP, 0x00},
	{MAX98396_R20F5_BPE_L0_ATTACK_REL_STEP, 0x00},
	{MAX98396_R20F6_BPE_L3_MAX_GAIN_ATTN, 0x00},
	{MAX98396_R20F7_BPE_L2_MAX_GAIN_ATTN, 0x00},
	{MAX98396_R20F8_BPE_L1_MAX_GAIN_ATTN, 0x00},
	{MAX98396_R20F9_BPE_L0_MAX_GAIN_ATTN, 0x00},
	{MAX98396_R20FA_BPE_L3_GAIN_ATTACK_REL_RATE, 0x00},
	{MAX98396_R20FB_BPE_L2_GAIN_ATTACK_REL_RATE, 0x00},
	{MAX98396_R20FC_BPE_L1_GAIN_ATTACK_REL_RATE, 0x00},
	{MAX98396_R20FD_BPE_L0_GAIN_ATTACK_REL_RATE, 0x00},
	{MAX98396_R20FE_BPE_L3_LIMITER_CFG, 0x00},
	{MAX98396_R20FF_BPE_L2_LIMITER_CFG, 0x00},
	{MAX98396_R2100_BPE_L1_LIMITER_CFG, 0x00},
	{MAX98396_R2101_BPE_L0_LIMITER_CFG, 0x00},
	{MAX98396_R2102_BPE_L3_LIMITER_ATTACK_REL_RATE, 0x00},
	{MAX98396_R2103_BPE_L2_LIMITER_ATTACK_REL_RATE, 0x00},
	{MAX98396_R2104_BPE_L1_LIMITER_ATTACK_REL_RATE, 0x00},
	{MAX98396_R2105_BPE_L0_LIMITER_ATTACK_REL_RATE, 0x00},
	{MAX98396_R2106_BPE_THRESH_HYSTERESIS, 0x00},
	{MAX98396_R2107_BPE_INFINITE_HOLD_CLR, 0x00},
	{MAX98396_R2108_BPE_SUPPLY_SRC, 0x00},
	{MAX98396_R2109_BPE_LOW_STATE, 0x00},
	{MAX98396_R210A_BPE_LOW_GAIN, 0x00},
	{MAX98396_R210B_BPE_LOW_LIMITER, 0x00},
	{MAX98396_R210D_BPE_EN, 0x00},
	{MAX98396_R210E_AUTO_RESTART_BEHAVIOR, 0x00},
	{MAX98396_R210F_GLOBAL_EN, 0x00},
	{MAX98396_R21FF_REVISION_ID, 0x00},
};

static int max98396_dai_set_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_component *component = codec_dai->component;
	struct max98396_priv *max98396 =
		snd_soc_component_get_drvdata(component);
	unsigned int format = 0;
	unsigned int invert = 0;

	pr_info("[RYAN] %s in, fmt : 0x%08x", __func__, fmt);
	dev_dbg(component->dev, "%s: fmt 0x%08X\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		invert = MAX98396_PCM_MODE_CFG_PCM_BCLKEDGE;
		break;
	default:
		dev_err(component->dev, "DAI invert mode unsupported\n");
		return -EINVAL;
	}

	regmap_update_bits(max98396->regmap,
		MAX98396_R2042_PCM_CLK_SETUP,
		MAX98396_PCM_MODE_CFG_PCM_BCLKEDGE,
		invert);

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		format = MAX98396_PCM_FORMAT_I2S;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		format = MAX98396_PCM_FORMAT_LJ;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		format = MAX98396_PCM_FORMAT_TDM_MODE1;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		format = MAX98396_PCM_FORMAT_TDM_MODE0;
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(max98396->regmap,
		MAX98396_R2041_PCM_MODE_CFG,
		MAX98396_PCM_MODE_CFG_FORMAT_MASK,
		format << MAX98396_PCM_MODE_CFG_FORMAT_SHIFT);

	pr_info("[RYAN] %s out, format : %d", __func__, format);
	return 0;
}

/* BCLKs per LRCLK */
static const int bclk_sel_table[] = {
	32, 48, 64, 96, 128, 192, 256, 384, 512, 320,
};

static int max98396_get_bclk_sel(int bclk)
{
	int i;
	/* match BCLKs per LRCLK */
	for (i = 0; i < ARRAY_SIZE(bclk_sel_table); i++) {
		if (bclk_sel_table[i] == bclk)
			return i + 2;
	}
	return 0;
}

static int max98396_set_clock(struct snd_soc_component *component,
	struct snd_pcm_hw_params *params)
{
	struct max98396_priv *max98396 =
		snd_soc_component_get_drvdata(component);
	/* BCLK/LRCLK ratio calculation */
	int blr_clk_ratio = params_channels(params) * max98396->ch_size;
	int value;

	if (!max98396->tdm_mode) {
		/* BCLK configuration */
		value = max98396_get_bclk_sel(blr_clk_ratio);
		pr_info("[RYAN] %s value:%d", __func__, value);
		if (!value) {
			dev_err(component->dev, "format unsupported %d\n",
				params_format(params));
			return -EINVAL;
		}

		regmap_update_bits(max98396->regmap,
			MAX98396_R2042_PCM_CLK_SETUP,
			MAX98396_PCM_CLK_SETUP_BSEL_MASK,
			value);
	}
	pr_info("[RYAN] %s out", __func__);
	return 0;
}

static int max98396_dai_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct max98396_priv *max98396 =
		snd_soc_component_get_drvdata(component);
	unsigned int sampling_rate = 0;
	unsigned int chan_sz = 0;

	/* pcm mode configuration */
	switch (snd_pcm_format_width(params_format(params))) {
	case 16:
		chan_sz = MAX98396_PCM_MODE_CFG_CHANSZ_16;
		break;
	case 24:
		chan_sz = MAX98396_PCM_MODE_CFG_CHANSZ_24;
		break;
	case 32:
		chan_sz = MAX98396_PCM_MODE_CFG_CHANSZ_32;
		break;
	default:
		dev_err(component->dev, "format unsupported %d\n",
			params_format(params));
		goto err;
	}

	max98396->ch_size = snd_pcm_format_width(params_format(params));

	regmap_update_bits(max98396->regmap,
		MAX98396_R2041_PCM_MODE_CFG,
		MAX98396_PCM_MODE_CFG_CHANSZ_MASK, chan_sz);

	dev_dbg(component->dev, "format supported %d",
		params_format(params));

	/* sampling rate configuration */
	switch (params_rate(params)) {
	case 8000:
		sampling_rate = MAX98396_PCM_SR_8000;
		break;
	case 11025:
		sampling_rate = MAX98396_PCM_SR_11025;
		break;
	case 12000:
		sampling_rate = MAX98396_PCM_SR_12000;
		break;
	case 16000:
		sampling_rate = MAX98396_PCM_SR_16000;
		break;
	case 22050:
		sampling_rate = MAX98396_PCM_SR_22050;
		break;
	case 24000:
		sampling_rate = MAX98396_PCM_SR_24000;
		break;
	case 32000:
		sampling_rate = MAX98396_PCM_SR_32000;
		break;
	case 44100:
		sampling_rate = MAX98396_PCM_SR_44100;
		break;
	case 48000:
		sampling_rate = MAX98396_PCM_SR_48000;
		break;
	case 88200:
		sampling_rate = MAX98396_PCM_SR_88200;
		break;
	case 96000:
		sampling_rate = MAX98396_PCM_SR_96000;
		break;
	default:
		dev_err(component->dev, "rate %d not supported\n",
			params_rate(params));
		goto err;
	}

	/* set DAI_SR to correct LRCLK frequency */
	regmap_update_bits(max98396->regmap,
		MAX98396_R2043_PCM_SR_SETUP,
		MAX98396_PCM_SR_MASK,
		sampling_rate);

	/* set sampling rate of IV */
	if (max98396->interleave_mode &&
	    sampling_rate > MAX98396_PCM_SR_16000)
		regmap_update_bits(max98396->regmap,
			MAX98396_R2043_PCM_SR_SETUP,
			MAX98396_IVADC_SR_MASK,
			(sampling_rate - 3) << MAX98396_IVADC_SR_SHIFT);
	else
		regmap_update_bits(max98396->regmap,
			MAX98396_R2043_PCM_SR_SETUP,
			MAX98396_IVADC_SR_MASK,
			sampling_rate << MAX98396_IVADC_SR_SHIFT);

	return max98396_set_clock(component, params);
err:
	pr_err("%s out error", __func__);
	return -EINVAL;
}

static int max98396_dai_tdm_slot(struct snd_soc_dai *dai,
	unsigned int tx_mask, unsigned int rx_mask,
	int slots, int slot_width)
{
	struct snd_soc_component *component = dai->component;
	struct max98396_priv *max98396 =
		snd_soc_component_get_drvdata(component);
	int bsel = 0;
	unsigned int chan_sz = 0;

	pr_info("[RYAN] %s rx_mask : 0x%08x, tx_mask : 0x%08x", __func__, rx_mask, tx_mask);
	
	if (!tx_mask && !rx_mask && !slots && !slot_width)
		max98396->tdm_mode = false;
	else
		max98396->tdm_mode = true;

	/* BCLK configuration */
	bsel = max98396_get_bclk_sel(slots * slot_width);
	if (bsel == 0) {
		dev_err(component->dev, "BCLK %d not supported\n",
			slots * slot_width);
		return -EINVAL;
	}

	regmap_update_bits(max98396->regmap,
		MAX98396_R2042_PCM_CLK_SETUP,
		MAX98396_PCM_CLK_SETUP_BSEL_MASK,
		bsel);

	/* Channel size configuration */
	switch (slot_width) {
	case 16:
		chan_sz = MAX98396_PCM_MODE_CFG_CHANSZ_16;
		break;
	case 24:
		chan_sz = MAX98396_PCM_MODE_CFG_CHANSZ_24;
		break;
	case 32:
		chan_sz = MAX98396_PCM_MODE_CFG_CHANSZ_32;
		break;
	default:
		dev_err(component->dev, "format unsupported %d\n",
			slot_width);
		return -EINVAL;
	}

	regmap_update_bits(max98396->regmap,
		MAX98396_R2041_PCM_MODE_CFG,
		MAX98396_PCM_MODE_CFG_CHANSZ_MASK, chan_sz);

	/* Rx slot configuration */
	regmap_update_bits(max98396->regmap,
		MAX98396_R2056_PCM_RX_SRC2,
		MAX98396_PCM_DMIX_CH0_SRC_MASK,
		rx_mask);
	regmap_update_bits(max98396->regmap,
		MAX98396_R2056_PCM_RX_SRC2,
		MAX98396_PCM_DMIX_CH1_SRC_MASK,
		rx_mask << MAX98396_PCM_DMIX_CH1_SHIFT);

	/* Tx slot Hi-Z configuration */
	regmap_write(max98396->regmap,
		MAX98396_R2053_PCM_TX_HIZ_CTRL_8,
		~tx_mask & 0xFF);
	regmap_write(max98396->regmap,
		MAX98396_R2052_PCM_TX_HIZ_CTRL_7,
		(~tx_mask & 0xFF00) >> 8);

	pr_info("[RYAN] %s out", __func__);
	return 0;
}

#define MAX98396_RATES SNDRV_PCM_RATE_8000_96000

#define MAX98396_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_ops max98396_dai_ops = {
	.set_fmt = max98396_dai_set_fmt,
	.hw_params = max98396_dai_hw_params,
	.set_tdm_slot = max98396_dai_tdm_slot,
};

static int max98396_dac_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component =
		snd_soc_dapm_to_component(w->dapm);
	struct max98396_priv *max98396 =
		snd_soc_component_get_drvdata(component);		
		
	pr_info("[RYAN] %s event : %d", __func__, event);
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(max98396->regmap,
			MAX98396_R205E_PCM_RX_EN,
			MAX98396_PCM_RX_EN_MASK, 1);

		regmap_write(max98396->regmap,
			MAX98396_R210F_GLOBAL_EN, 1);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(max98396->regmap,
			MAX98396_R205E_PCM_RX_EN,
			MAX98396_PCM_RX_EN_MASK, 0);

		regmap_write(max98396->regmap,
			MAX98396_R210F_GLOBAL_EN, 0);
		max98396->tdm_mode = false;
		break;
	default:
		return 0;
	}
	
	return 0;
}

static const char * const max98396_switch_text[] = {
	"Left", "Right", "LeftRight"};

static const struct soc_enum dai_sel_enum =
	SOC_ENUM_SINGLE(MAX98396_R2055_PCM_RX_SRC1,
		0, 3, max98396_switch_text);

static const struct snd_kcontrol_new max98396_dai_controls =
	SOC_DAPM_ENUM("DAI Sel", dai_sel_enum);

static const struct snd_kcontrol_new max98396_vi_control =
	SOC_DAPM_SINGLE("Switch", MAX98396_R205F_PCM_TX_EN, 0, 1, 0);

static const struct snd_soc_dapm_widget max98396_dapm_widgets[] = {
SND_SOC_DAPM_DAC_E("Amp Enable", "HiFi Playback",
	MAX98396_R20AF_AMP_EN, 0, 0, max98396_dac_event,
	SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
SND_SOC_DAPM_MUX("DAI Sel Mux", SND_SOC_NOPM, 0, 0,
	&max98396_dai_controls),
SND_SOC_DAPM_OUTPUT("BE_OUT"),
SND_SOC_DAPM_AIF_OUT("Voltage Sense", "HiFi Capture", 0,
	MAX98396_R20E4_IV_SENSE_PATH_EN, 0, 0),
SND_SOC_DAPM_AIF_OUT("Current Sense", "HiFi Capture", 0,
	MAX98396_R20E4_IV_SENSE_PATH_EN, 1, 0),
SND_SOC_DAPM_SWITCH("VI Sense", SND_SOC_NOPM, 0, 0,
	&max98396_vi_control),
SND_SOC_DAPM_SIGGEN("VMON"),
SND_SOC_DAPM_SIGGEN("IMON"),
SND_SOC_DAPM_SIGGEN("FBMON"),
};

static DECLARE_TLV_DB_SCALE(max98396_digital_tlv, -6300, 50, 1);
static const DECLARE_TLV_DB_RANGE(max98396_spk_tlv,
	0, 17, TLV_DB_SCALE_ITEM(400, 100, 0),
);

static bool max98396_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98396_R2027_THERM_FOLDBACK_EN:
	case MAX98396_R2030_NOISE_GATE_IDLE_MODE_CTRL:
	case MAX98396_R2033_NOISE_GATE_IDLE_MODE_EN:
	case MAX98396_R2058_PCM_BYPASS_SRC:
	case MAX98396_R207F_ICC_EN:
	case MAX98396_R20C7_MEAS_ADC_CFG:
	case MAX98396_R20E0_IV_SENSE_PATH_CFG:
	case MAX98396_R21FF_REVISION_ID:
	case MAX98396_R2001_INT_RAW1 ... MAX98396_R2013_INT_EN4:
	case MAX98396_R201F_IRQ_CTRL ... MAX98396_R2024_THERM_FOLDBACK_SET:
	case MAX98396_R2038_CLK_MON_CTRL ... MAX98396_R2053_PCM_TX_HIZ_CTRL_8:
	case MAX98396_R2055_PCM_RX_SRC1 ... MAX98396_R2056_PCM_RX_SRC2:
	case MAX98396_R205D_PCM_TX_SRC_EN ... MAX98396_R205F_PCM_TX_EN:
	case MAX98396_R2070_ICC_RX_EN_A... MAX98396_R2072_ICC_TX_CTRL:
	case MAX98396_R2083_TONE_GEN_DC_CFG ... MAX98396_R2086_TONE_GEN_DC_LVL3:
	case MAX98396_R208F_TONE_GEN_EN ... MAX98396_R209A_SPK_EDGE_CTRL:
	case MAX98396_R209C_SPK_EDGE_CTRL1 ... MAX98396_R20A0_AMP_SUPPLY_CTL:
	case MAX98396_R20AF_AMP_EN ... MAX98396_R20BF_LO_VBAT_READBACK_LSB:
	case MAX98396_R20D0_DHT_CFG1 ... MAX98396_R20D6_DHT_HYSTERESIS_CFG:
	case MAX98396_R20E4_IV_SENSE_PATH_EN
		... MAX98396_R2106_BPE_THRESH_HYSTERESIS:
	case MAX98396_R2108_BPE_SUPPLY_SRC ... MAX98396_R210B_BPE_LOW_LIMITER:
	case MAX98396_R210D_BPE_EN ... MAX98396_R210F_GLOBAL_EN:
		return true;
	default:
		return false;
	}
};

static bool max98396_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98396_R210F_GLOBAL_EN:
	case MAX98396_R21FF_REVISION_ID:
	case MAX98396_R2001_INT_RAW1 ... MAX98396_R200E_INT_FLAG4:
	case MAX98396_R20B6_ADC_PVDD_READBACK_MSB
		... MAX98396_R20BF_LO_VBAT_READBACK_LSB:
		return true;
	default:
		return false;
	}
}

static const struct snd_kcontrol_new max98396_snd_controls[] = {
/* Volume */
SOC_SINGLE_TLV("Digital Volume", MAX98396_R2090_AMP_VOL_CTRL,
	0, 0x7F, 1, max98396_digital_tlv),
SOC_SINGLE_TLV("Speaker Volume", MAX98396_R2091_AMP_PATH_GAIN,
	0, 0x11, 0, max98396_spk_tlv),
/* Volume Ramp Up/Down Enable*/
SOC_SINGLE("Ramp Up Switch", MAX98396_R2092_AMP_DSP_CFG,
	MAX98396_DSP_SPK_VOL_RMPUP_SHIFT, 1, 0),
SOC_SINGLE("Ramp Down Switch", MAX98396_R2092_AMP_DSP_CFG,
	MAX98396_DSP_SPK_VOL_RMPDN_SHIFT, 1, 0),
/* Clock Monitor Enable */
SOC_SINGLE("CLK Monitor Switch", MAX98396_R203F_ENABLE_CTRLS,
	MAX98396_CTRL_CMON_EN_SHIFT, 1, 0),
/* Dither Enable */
SOC_SINGLE("Dither Switch", MAX98396_R2092_AMP_DSP_CFG,
	MAX98396_DSP_SPK_DITH_EN_SHIFT, 1, 0),
/* DC Blocker Enable */
SOC_SINGLE("DC Blocker Switch", MAX98396_R2092_AMP_DSP_CFG,
	MAX98396_DSP_SPK_DCBLK_EN_SHIFT, 1, 0),
/* PCM Bypass Enable */
SOC_SINGLE("PCM Bypass Switch", MAX98396_R205E_PCM_RX_EN, 1, 1, 0),
/* Dynamic Headroom Tracking */
SOC_SINGLE("DHT Switch", MAX98396_R20DF_DHT_EN, 0, 1, 0),
/* Brownout Protection Engine */
SOC_SINGLE("BPE Switch", MAX98396_R210D_BPE_EN, 0, 1, 0),
SOC_SINGLE("BPE Limiter Switch", MAX98396_R210D_BPE_EN, 1, 1, 0),
};

static const struct snd_soc_dapm_route max98396_audio_map[] = {
	/* Plabyack */
	{"DAI Sel Mux", "Left", "Amp Enable"},
	{"DAI Sel Mux", "Right", "Amp Enable"},
	{"DAI Sel Mux", "LeftRight", "Amp Enable"},
	{"BE_OUT", NULL, "DAI Sel Mux"},
	/* Capture */
	{ "VI Sense", "Switch", "VMON" },
	{ "VI Sense", "Switch", "IMON" },
	{ "Voltage Sense", NULL, "VI Sense" },
	{ "Current Sense", NULL, "VI Sense" },
};

static struct snd_soc_dai_driver max98396_dai[] = {
	{
		.name = "max98396-aif1",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98396_RATES,
			.formats = MAX98396_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98396_RATES,
			.formats = MAX98396_FORMATS,
		},
		.ops = &max98396_dai_ops,
	}
};

static void max98396_reset(struct max98396_priv *max98396, struct device *dev)
{
	int ret, reg, count;

	pr_info("[RYAN] %s in", __func__);
	/* Software Reset */
	ret = regmap_write(max98396->regmap,
		MAX98396_R2000_SW_RESET, 1);
	if (ret)
		dev_err(dev, "Reset command failed. (ret:%d)\n", ret);

	count = 0;
	while (count < 3) {
		usleep_range(10000, 11000);
		/* Software Reset Verification */
		ret = regmap_read(max98396->regmap,
			MAX98396_R21FF_REVISION_ID, &reg);
		if (!ret) {
			dev_info(dev, "Reset completed (retry:%d)\n", count);
			return;
		}
		count++;
	}
	dev_err(dev, "Reset failed. (ret:%d)\n", ret);
}

static int max98396_probe(struct snd_soc_component *component)
{
	struct max98396_priv *max98396 =
		snd_soc_component_get_drvdata(component);

	pr_info("[RYAN] %s in", __func__);
	/* Software Reset */
	max98396_reset(max98396, component->dev);

	/* disable all monitoring */
	regmap_write(max98396->regmap,
		MAX98396_R203F_ENABLE_CTRLS, 0x00);
	
	/* L/R mix configuration */
	regmap_write(max98396->regmap,
		MAX98396_R2055_PCM_RX_SRC1, 0x00);

	regmap_write(max98396->regmap,
		MAX98396_R2056_PCM_RX_SRC2, 0x10);
	/* Tx enable */
	regmap_write(max98396->regmap,
		MAX98396_R205F_PCM_TX_EN, 1);
	/* PCM V MON enable */
	regmap_write(max98396->regmap,
		MAX98396_R205D_PCM_TX_SRC_EN, 1);		
	/* Enable DC blocker */
	regmap_update_bits(max98396->regmap,
		MAX98396_R2092_AMP_DSP_CFG, 0x21, 0x01);
	/* Enable IMON VMON DC blocker */
	regmap_update_bits(max98396->regmap,
		MAX98396_R20E0_IV_SENSE_PATH_CFG, 3, 3);
	/* voltage, current slot configuration */
	regmap_write(max98396->regmap,
		MAX98396_R2044_PCM_TX_CTRL_1,
		max98396->v_slot);
	regmap_write(max98396->regmap,
		MAX98396_R2045_PCM_TX_CTRL_2,
		max98396->i_slot);

	if (max98396->v_slot < 8)
		regmap_update_bits(max98396->regmap,
			MAX98396_R2053_PCM_TX_HIZ_CTRL_8,
			1 << max98396->v_slot, 0);
	else
		regmap_update_bits(max98396->regmap,
			MAX98396_R2052_PCM_TX_HIZ_CTRL_7,
			1 << (max98396->v_slot - 8), 0);

	if (max98396->i_slot < 8)
		regmap_update_bits(max98396->regmap,
			MAX98396_R2053_PCM_TX_HIZ_CTRL_8,
			1 << max98396->i_slot, 0);
	else
		regmap_update_bits(max98396->regmap,
			MAX98396_R2052_PCM_TX_HIZ_CTRL_7,
			1 << (max98396->i_slot - 8), 0);

	/* Set interleave mode */
	if (max98396->interleave_mode)
		regmap_update_bits(max98396->regmap,
			MAX98396_R2041_PCM_MODE_CFG,
			MAX98396_PCM_TX_CH_INTERLEAVE_MASK,
			MAX98396_PCM_TX_CH_INTERLEAVE_MASK);

	regmap_update_bits(max98396->regmap,
		MAX98396_R2038_CLK_MON_CTRL,
		MAX98396_CLK_MON_AUTO_RESTART_MASK,
		MAX98396_CLK_MON_AUTO_RESTART_MASK);

	pr_info("[RYAN] %s out", __func__);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int max98396_suspend(struct device *dev)
{
	struct max98396_priv *max98396 = dev_get_drvdata(dev);

	regcache_cache_only(max98396->regmap, true);
	regcache_mark_dirty(max98396->regmap);
	return 0;
}
static int max98396_resume(struct device *dev)
{
	struct max98396_priv *max98396 = dev_get_drvdata(dev);

	regcache_cache_only(max98396->regmap, false);
	max98396_reset(max98396, dev);
	regcache_sync(max98396->regmap);
	return 0;
}
#endif

static const struct dev_pm_ops max98396_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(max98396_suspend, max98396_resume)
};

static const struct snd_soc_component_driver soc_codec_dev_max98396 = {
	.probe			= max98396_probe,
	.controls		= max98396_snd_controls,
	.num_controls		= ARRAY_SIZE(max98396_snd_controls),
	.dapm_widgets		= max98396_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(max98396_dapm_widgets),
	.dapm_routes		= max98396_audio_map,
	.num_dapm_routes	= ARRAY_SIZE(max98396_audio_map),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static const struct regmap_config max98396_regmap = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = MAX98396_R21FF_REVISION_ID,
	.reg_defaults  = max98396_reg,
	.num_reg_defaults = ARRAY_SIZE(max98396_reg),
	.readable_reg = max98396_readable_register,
	.volatile_reg = max98396_volatile_reg,
	.cache_type = REGCACHE_RBTREE,
};

static void max98396_slot_config(struct i2c_client *i2c,
	struct max98396_priv *max98396)
{
	int value;
	struct device *dev = &i2c->dev;

	if (!device_property_read_u32(dev, "maxim,vmon-slot-no", &value))
		max98396->v_slot = value & 0xF;
	else
		max98396->v_slot = 0;

	if (!device_property_read_u32(dev, "maxim,imon-slot-no", &value))
		max98396->i_slot = value & 0xF;
	else
		max98396->i_slot = 1;
	if (dev->of_node) {
		max98396->reset_gpio = of_get_named_gpio(dev->of_node,
						"maxim,reset-gpio", 0);
		if (!gpio_is_valid(max98396->reset_gpio)) {
			dev_err(dev, "Looking up %s property in node %s failed %d\n",
				"maxim,reset-gpio", dev->of_node->full_name,
				max98396->reset_gpio);
		} else {
			dev_dbg(dev, "maxim,reset-gpio=%d",
				max98396->reset_gpio);
		}
	} else {
		/* this makes reset_gpio as invalid */
		max98396->reset_gpio = -1;
	}
}

static int max98396_i2c_probe(struct i2c_client *i2c,
	const struct i2c_device_id *id)
{

	int ret = 0;
	int reg = 0;
	struct max98396_priv *max98396 = NULL;

	pr_info("[RYAN] %s in", __func__);
	max98396 = devm_kzalloc(&i2c->dev, sizeof(*max98396), GFP_KERNEL);

	if (!max98396) {
		ret = -ENOMEM;
		return ret;
	}
	i2c_set_clientdata(i2c, max98396);

	/* update interleave mode info */
	if (device_property_read_bool(&i2c->dev, "maxim,interleave_mode"))
		max98396->interleave_mode = true;
	else
		max98396->interleave_mode = false;

	/* regmap initialization */
	max98396->regmap
		= devm_regmap_init_i2c(i2c, &max98396_regmap);
	if (IS_ERR(max98396->regmap)) {
		ret = PTR_ERR(max98396->regmap);
		dev_err(&i2c->dev,
			"Failed to allocate regmap: %d\n", ret);
		return ret;
	}

	/* voltage/current slot & gpio configuration */
	max98396_slot_config(i2c, max98396);

	/* Power on device */
	if (gpio_is_valid(max98396->reset_gpio)) {
		ret = devm_gpio_request(&i2c->dev, max98396->reset_gpio,
					"MAX98396_RESET");
		if (ret) {
			dev_err(&i2c->dev, "%s: Failed to request gpio %d\n",
				__func__, max98396->reset_gpio);
			return -EINVAL;
		}
		gpio_direction_output(max98396->reset_gpio, 0);
		msleep(50);
		gpio_direction_output(max98396->reset_gpio, 1);
		msleep(20);
	}

	/* Check Revision ID */
	ret = regmap_read(max98396->regmap,
		MAX98396_R21FF_REVISION_ID, &reg);
	if (ret < 0) {
		dev_err(&i2c->dev,
			"Failed to read: 0x%02X\n", MAX98396_R21FF_REVISION_ID);
		return ret;
	}
	dev_info(&i2c->dev, "MAX98396 revisionID: 0x%02X\n", reg);

	/* codec registration */
	ret = devm_snd_soc_register_component(&i2c->dev,
					      &soc_codec_dev_max98396,
		max98396_dai, ARRAY_SIZE(max98396_dai));
	if (ret < 0)
		dev_err(&i2c->dev, "Failed to register codec: %d\n", ret);

	pr_info("[RYAN] %s out", __func__);
	return ret;
}

static const struct i2c_device_id max98396_i2c_id[] = {
	{ "max98396", 0},
	{ },
};

MODULE_DEVICE_TABLE(i2c, max98396_i2c_id);

#if defined(CONFIG_OF)
static const struct of_device_id max98396_of_match[] = {
	{ .compatible = "maxim,max98396", },
	{ }
};
MODULE_DEVICE_TABLE(of, max98396_of_match);
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id max98396_acpi_match[] = {
	{ "MX98396", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, max98396_acpi_match);
#endif

static struct i2c_driver max98396_i2c_driver = {
	.driver = {
		.name = "max98396",
		.of_match_table = of_match_ptr(max98396_of_match),
		.acpi_match_table = ACPI_PTR(max98396_acpi_match),
		.pm = &max98396_pm,
	},
	.probe = max98396_i2c_probe,
	.id_table = max98396_i2c_id,
};

module_i2c_driver(max98396_i2c_driver)

MODULE_DESCRIPTION("ALSA SoC MAX98396 driver");
MODULE_AUTHOR("Ryan Lee <ryans.lee@maximintegrated.com>");
MODULE_LICENSE("GPL");
