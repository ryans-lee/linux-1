// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021, Maxim Integrated

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
#include "max98520.h"

static struct reg_default max98520_reg[] = {
	{MAX98520_R2000_SW_RESET, 0x00},
	{MAX98520_R2001_STATUS_1, 0x00},
	{MAX98520_R2002_STATUS_2, 0x00},
	{MAX98520_R2020_THERM_WARN_THRESH, 0x46},
	{MAX98520_R2021_THERM_SHDN_THRESH, 0x64},
	{MAX98520_R2022_THERM_HYSTERESIS, 0x02},
	{MAX98520_R2023_THERM_FOLDBACK_SET, 0x31},
	{MAX98520_R2027_THERM_FOLDBACK_EN, 0x01},
	{MAX98520_R2030_CLK_MON_CTRL, 0x00},
	{MAX98520_R2037_ERR_MON_CTRL, 0x01},
	{MAX98520_R2040_PCM_MODE_CFG, 0xC0},
	{MAX98520_R2041_PCM_CLK_SETUP, 0x04},
	{MAX98520_R2042_PCM_SR_SETUP, 0x08},
	{MAX98520_R2043_PCM_RX_SRC1, 0x00},
	{MAX98520_R2044_PCM_RX_SRC2, 0x00},
	{MAX98520_R204F_PCM_RX_EN, 0x00},
	{MAX98520_R2090_AMP_VOL_CTRL, 0x00},
	{MAX98520_R2091_AMP_PATH_GAIN, 0x03},
	{MAX98520_R2092_AMP_DSP_CFG, 0x02},
	{MAX98520_R2094_SSM_CFG, 0x01},
	{MAX98520_R2095_AMP_CFG, 0xF0},
	{MAX98520_R209F_AMP_EN, 0x00},
	{MAX98520_R20B0_ADC_SR, 0x00},
	{MAX98520_R20B1_ADC_RESOLUTION, 0x00},
	{MAX98520_R20B2_ADC_PVDD0_CFG, 0x02},
	{MAX98520_R20B3_ADC_THERMAL_CFG, 0x02},
	{MAX98520_R20B4_ADC_READBACK_CTRL, 0x00},
	{MAX98520_R20B5_ADC_READBACK_UPDATE, 0x00},
	{MAX98520_R20B6_ADC_PVDD_READBACK_MSB, 0x00},
	{MAX98520_R20B7_ADC_PVDD_READBACK_LSB, 0x00},
	{MAX98520_R20B8_ADC_TEMP_READBACK_MSB, 0x00},
	{MAX98520_R20B9_ADC_TEMP_READBACK_LSB, 0x00},
	{MAX98520_R20BA_ADC_LOW_PVDD_READBACK_MSB, 0xFF},
	{MAX98520_R20BB_ADC_LOW_READBACK_LSB, 0x01},
	{MAX98520_R20BC_ADC_HIGH_TEMP_READBACK_MSB, 0x00},
	{MAX98520_R20BD_ADC_HIGH_TEMP_READBACK_LSB, 0x00},
	{MAX98520_R20CF_MEAS_ADC_CFG, 0x00},
	{MAX98520_R20D0_DHT_CFG1, 0x00},
	{MAX98520_R20D1_LIMITER_CFG1, 0x08},
	{MAX98520_R20D2_LIMITER_CFG2, 0x00},
	{MAX98520_R20D3_DHT_CFG2, 0x14},
	{MAX98520_R20D4_DHT_CFG3, 0x02},
	{MAX98520_R20D5_DHT_CFG4, 0x04},
	{MAX98520_R20D6_DHT_HYSTERESIS_CFG, 0x07},
	{MAX98520_R20D8_DHT_EN, 0x00},
	{MAX98520_R210E_AUTO_RESTART_BEHAVIOR, 0x00},
	{MAX98520_R210F_GLOBAL_EN, 0x00},
	{MAX98520_R21FF_REVISION_ID, 0x00},
};

static int max98520_dai_set_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_component *component = codec_dai->component;
	struct max98520_priv *max98520 =
		snd_soc_component_get_drvdata(component);
	unsigned int format = 0;
	unsigned int invert = 0;

	pr_info("[MAX98520_DEBUG] %s in", __func__);
	dev_dbg(component->dev, "%s: fmt 0x%08X\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		invert = MAX98520_PCM_MODE_CFG_PCM_BCLKEDGE;
		break;
	default:
		dev_err(component->dev, "DAI invert mode unsupported\n");
		return -EINVAL;
	}

	regmap_update_bits(max98520->regmap,
		MAX98520_R2041_PCM_CLK_SETUP,
		MAX98520_PCM_MODE_CFG_PCM_BCLKEDGE,
		invert);

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		format = MAX98520_PCM_FORMAT_I2S;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		format = MAX98520_PCM_FORMAT_LJ;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		format = MAX98520_PCM_FORMAT_TDM_MODE1;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		format = MAX98520_PCM_FORMAT_TDM_MODE0;
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(max98520->regmap,
		MAX98520_R2040_PCM_MODE_CFG,
		MAX98520_PCM_MODE_CFG_FORMAT_MASK,
		format << MAX98520_PCM_MODE_CFG_FORMAT_SHIFT);

	pr_info("[MAX98520_DEBUG] %s fmt:%x out", __func__, fmt);
	return 0;
}

/* BCLKs per LRCLK */
static const int bclk_sel_table[] = {
	32, 48, 64, 96, 128, 192, 256, 384, 512, 320,
};

static int max98520_get_bclk_sel(int bclk)
{
	int i;
	/* match BCLKs per LRCLK */
	for (i = 0; i < ARRAY_SIZE(bclk_sel_table); i++) {
		if (bclk_sel_table[i] == bclk)
			return i + 2;
	}
	return 0;
}

static int max98520_set_clock(struct snd_soc_component *component,
	struct snd_pcm_hw_params *params)
{
	struct max98520_priv *max98520 =
		snd_soc_component_get_drvdata(component);
	/* BCLK/LRCLK ratio calculation */
	int blr_clk_ratio = params_channels(params) * max98520->ch_size;
	int value;

	if (!max98520->tdm_mode) {
		/* BCLK configuration */
		value = max98520_get_bclk_sel(blr_clk_ratio);
		pr_info("[MAX98520_DEBUG] %s value:%d", __func__, value);
		if (!value) {
			dev_err(component->dev, "format unsupported %d\n",
				params_format(params));
			return -EINVAL;
		}

		regmap_update_bits(max98520->regmap,
			MAX98520_R2041_PCM_CLK_SETUP,
			MAX98520_PCM_CLK_SETUP_BSEL_MASK,
			value);
	}
	pr_info("[MAX98520_DEBUG] %s tdm_mode:%d out", __func__, max98520->tdm_mode);
	return 0;
}

static int max98520_dai_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct max98520_priv *max98520 =
		snd_soc_component_get_drvdata(component);
	unsigned int sampling_rate = 0;
	unsigned int chan_sz = 0;

	/* pcm mode configuration */
	switch (snd_pcm_format_width(params_format(params))) {
	case 16:
		chan_sz = MAX98520_PCM_MODE_CFG_CHANSZ_16;
		break;
	case 24:
		chan_sz = MAX98520_PCM_MODE_CFG_CHANSZ_24;
		break;
	case 32:
		chan_sz = MAX98520_PCM_MODE_CFG_CHANSZ_32;
		break;
	default:
		dev_err(component->dev, "format unsupported %d\n",
			params_format(params));
		goto err;
	}

	max98520->ch_size = snd_pcm_format_width(params_format(params));

	regmap_update_bits(max98520->regmap,
		MAX98520_R2040_PCM_MODE_CFG,
		MAX98520_PCM_MODE_CFG_CHANSZ_MASK, chan_sz);

	dev_dbg(component->dev, "format supported %d",
		params_format(params));

	/* sampling rate configuration */
	switch (params_rate(params)) {
	case 8000:
		sampling_rate = MAX98520_PCM_SR_8000;
		break;
	case 11025:
		sampling_rate = MAX98520_PCM_SR_11025;
		break;
	case 12000:
		sampling_rate = MAX98520_PCM_SR_12000;
		break;
	case 16000:
		sampling_rate = MAX98520_PCM_SR_16000;
		break;
	case 22050:
		sampling_rate = MAX98520_PCM_SR_22050;
		break;
	case 24000:
		sampling_rate = MAX98520_PCM_SR_24000;
		break;
	case 32000:
		sampling_rate = MAX98520_PCM_SR_32000;
		break;
	case 44100:
		sampling_rate = MAX98520_PCM_SR_44100;
		break;
	case 48000:
		sampling_rate = MAX98520_PCM_SR_48000;
		break;
	case 88200:
		sampling_rate = MAX98520_PCM_SR_88200;
		break;
	case 96000:
		sampling_rate = MAX98520_PCM_SR_96000;
		break;
	case 176400:
		sampling_rate = MAX98520_PCM_SR_176400;
		break;
	case 192000:
		sampling_rate = MAX98520_PCM_SR_192000;
		break;
	default:
		dev_err(component->dev, "rate %d not supported\n",
			params_rate(params));
		goto err;
	}

	pr_info("[MAX98520_DEBUG] %s ch_size: %d, sampling rate : %d out\n", __func__,
		snd_pcm_format_width(params_format(params)), params_rate(params));
	/* set DAI_SR to correct LRCLK frequency */
	regmap_update_bits(max98520->regmap,
		MAX98520_R2042_PCM_SR_SETUP,
		MAX98520_PCM_SR_MASK,
		sampling_rate);

	return max98520_set_clock(component, params);
err:
	pr_err("%s out error", __func__);
	return -EINVAL;
}

static int max98520_dai_tdm_slot(struct snd_soc_dai *dai,
	unsigned int tx_mask, unsigned int rx_mask,
	int slots, int slot_width)
{
	struct snd_soc_component *component = dai->component;
	struct max98520_priv *max98520 =
		snd_soc_component_get_drvdata(component);
	int bsel = 0;
	unsigned int chan_sz = 0;

	if (!tx_mask && !rx_mask && !slots && !slot_width)
		max98520->tdm_mode = false;
	else
		max98520->tdm_mode = true;

	/* BCLK configuration */
	bsel = max98520_get_bclk_sel(slots * slot_width);
	if (bsel == 0) {
		dev_err(component->dev, "BCLK %d not supported\n",
			slots * slot_width);
		return -EINVAL;
	}

	regmap_update_bits(max98520->regmap,
		MAX98520_R2041_PCM_CLK_SETUP,
		MAX98520_PCM_CLK_SETUP_BSEL_MASK,
		bsel);

	/* Channel size configuration */
	switch (slot_width) {
	case 16:
		chan_sz = MAX98520_PCM_MODE_CFG_CHANSZ_16;
		break;
	case 24:
		chan_sz = MAX98520_PCM_MODE_CFG_CHANSZ_24;
		break;
	case 32:
		chan_sz = MAX98520_PCM_MODE_CFG_CHANSZ_32;
		break;
	default:
		dev_err(component->dev, "format unsupported %d\n",
			slot_width);
		return -EINVAL;
	}

	regmap_update_bits(max98520->regmap,
		MAX98520_R2040_PCM_MODE_CFG,
		MAX98520_PCM_MODE_CFG_CHANSZ_MASK, chan_sz);

	/* Rx slot configuration */
	regmap_update_bits(max98520->regmap,
		MAX98520_R2044_PCM_RX_SRC2,
		MAX98520_PCM_DMIX_CH0_SRC_MASK,
		rx_mask);
	regmap_update_bits(max98520->regmap,
		MAX98520_R2044_PCM_RX_SRC2,
		MAX98520_PCM_DMIX_CH1_SRC_MASK,
		rx_mask << MAX98520_PCM_DMIX_CH1_SHIFT);

	return 0;
}

#define MAX98520_RATES SNDRV_PCM_RATE_8000_192000

#define MAX98520_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_ops max98520_dai_ops = {
	.set_fmt = max98520_dai_set_fmt,
	.hw_params = max98520_dai_hw_params,
	.set_tdm_slot = max98520_dai_tdm_slot,
};

static int max98520_dac_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component =
		snd_soc_dapm_to_component(w->dapm);
	struct max98520_priv *max98520 =
		snd_soc_component_get_drvdata(component);


	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		pr_info("[MAX98520_DEBUG] AMP ON\n");
		regmap_update_bits(max98520->regmap,
			MAX98520_R204F_PCM_RX_EN,
			MAX98520_PCM_RX_EN_MASK, 1);

		regmap_write(max98520->regmap,
			MAX98520_R210F_GLOBAL_EN, 1);
		usleep_range(30000, 31000);
		break;
	case SND_SOC_DAPM_POST_PMD:
		pr_info("[MAX98520_DEBUG] AMP OFF\n");
		regmap_update_bits(max98520->regmap,
			MAX98520_R204F_PCM_RX_EN,
			MAX98520_PCM_RX_EN_MASK, 0);

		regmap_write(max98520->regmap,
			MAX98520_R210F_GLOBAL_EN, 0);
		usleep_range(30000, 31000);
		max98520->tdm_mode = false;
		break;
	default:
		return 0;
	}
	return 0;
}

static const char * const max98520_switch_text[] = {
	"Left", "Right", "LeftRight"};

static const struct soc_enum dai_sel_enum =
	SOC_ENUM_SINGLE(MAX98520_R2043_PCM_RX_SRC1,
		0, 3, max98520_switch_text);

static const struct snd_kcontrol_new max98520_dai_controls =
	SOC_DAPM_ENUM("DAI Sel", dai_sel_enum);

static const struct snd_soc_dapm_widget max98520_dapm_widgets[] = {
SND_SOC_DAPM_DAC_E("Amp Enable", "HiFi Playback",
	MAX98520_R209F_AMP_EN, 0, 0, max98520_dac_event,
	SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
SND_SOC_DAPM_MUX("DAI Sel Mux", SND_SOC_NOPM, 0, 0,	&max98520_dai_controls),
SND_SOC_DAPM_OUTPUT("BE_OUT"),
};

static DECLARE_TLV_DB_SCALE(max98520_digital_tlv, -6300, 50, 1);
static const DECLARE_TLV_DB_RANGE(max98520_spk_tlv,
	0, 17, TLV_DB_SCALE_ITEM(400, 100, 0),
);

static bool max98520_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98520_R2027_THERM_FOLDBACK_EN:
	case MAX98520_R2030_CLK_MON_CTRL:
	case MAX98520_R2037_ERR_MON_CTRL:
	case MAX98520_R204F_PCM_RX_EN:
	case MAX98520_R209F_AMP_EN:
	case MAX98520_R20CF_MEAS_ADC_CFG:
	case MAX98520_R20D8_DHT_EN:
	case MAX98520_R21FF_REVISION_ID:
	case MAX98520_R2001_STATUS_1... MAX98520_R2002_STATUS_2:
	case MAX98520_R2020_THERM_WARN_THRESH... MAX98520_R2023_THERM_FOLDBACK_SET:
	case MAX98520_R2040_PCM_MODE_CFG... MAX98520_R2044_PCM_RX_SRC2:
	case MAX98520_R2090_AMP_VOL_CTRL... MAX98520_R2092_AMP_DSP_CFG:
	case MAX98520_R2094_SSM_CFG... MAX98520_R2095_AMP_CFG:
	case MAX98520_R20B0_ADC_SR... MAX98520_R20BD_ADC_HIGH_TEMP_READBACK_LSB:
	case MAX98520_R20D0_DHT_CFG1... MAX98520_R20D6_DHT_HYSTERESIS_CFG:
	case MAX98520_R210E_AUTO_RESTART_BEHAVIOR... MAX98520_R210F_GLOBAL_EN:
	case MAX98520_R2161_BOOST_TM1... MAX98520_R2163_BOOST_TM3:
		return true;
	default:
		return false;
	}
};

static bool max98520_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98520_R210F_GLOBAL_EN:
	case MAX98520_R21FF_REVISION_ID:
	case MAX98520_R2001_STATUS_1 ... MAX98520_R2002_STATUS_2:
	case MAX98520_R20B4_ADC_READBACK_CTRL
		... MAX98520_R20BD_ADC_HIGH_TEMP_READBACK_LSB:
		return true;
	default:
		return false;
	}
}

static const struct snd_kcontrol_new max98520_snd_controls[] = {
/* Volume */
SOC_SINGLE_TLV("Digital Volume", MAX98520_R2090_AMP_VOL_CTRL,
	0, 0x7F, 1, max98520_digital_tlv),
SOC_SINGLE_TLV("Speaker Volume", MAX98520_R2091_AMP_PATH_GAIN,
	0, 0x11, 0, max98520_spk_tlv),
/* Volume Ramp Up/Down Enable*/
SOC_SINGLE("Ramp Up Switch", MAX98520_R2092_AMP_DSP_CFG,
	MAX98520_DSP_SPK_VOL_RMPUP_SHIFT, 1, 0),
SOC_SINGLE("Ramp Down Switch", MAX98520_R2092_AMP_DSP_CFG,
	MAX98520_DSP_SPK_VOL_RMPDN_SHIFT, 1, 0),
/* Clock Monitor Enable */
SOC_SINGLE("CLK Monitor Switch", MAX98520_R2037_ERR_MON_CTRL,
	   MAX98520_CTRL_CMON_EN_SHIFT, 1, 0),
/* Clock Monitor Config */
SOC_SINGLE("CLKMON Autorestart Switch", MAX98520_R2030_CLK_MON_CTRL,
	   MAX98520_CMON_AUTORESTART_SHIFT, 1, 0),
/* Dither Enable */
SOC_SINGLE("Dither Switch", MAX98520_R2092_AMP_DSP_CFG,
	   MAX98520_DSP_SPK_DITH_EN_SHIFT, 1, 0),
/* DC Blocker Enable */
SOC_SINGLE("DC Blocker Switch", MAX98520_R2092_AMP_DSP_CFG,
	   MAX98520_DSP_SPK_DCBLK_EN_SHIFT, 1, 0),
/* Speaker Safe Mode Enable */
SOC_SINGLE("Speaker Safemode Switch", MAX98520_R2092_AMP_DSP_CFG,
	   MAX98520_DSP_SPK_SAFE_EN_SHIFT, 1, 0),
/* AMP SSM Enable */
SOC_SINGLE("CP Bypass Switch", MAX98520_R2094_SSM_CFG,
	   MAX98520_SSM_RCVR_MODE_SHIFT, 1, 0),
/* AMP Dynamic Mode Configuration */
SOC_SINGLE("Dynamic Mode Switch", MAX98520_R2095_AMP_CFG,
	   MAX98520_CFG_DYN_MODE_SHIFT, 1, 1),
/* AMP Speaker Mode Switch */ 
SOC_SINGLE("Speaker Mode Switch", MAX98520_R2095_AMP_CFG,
	   MAX98520_CFG_SPK_MODE_SHIFT, 1, 0),
/* Dynamic Headroom Tracking */
SOC_SINGLE("DHT Switch", MAX98520_R20D8_DHT_EN, 0, 1, 0),
};

static const struct snd_soc_dapm_route max98520_audio_map[] = {
	/* Plabyack */
	{"DAI Sel Mux", "Left", "Amp Enable"},
	{"DAI Sel Mux", "Right", "Amp Enable"},
	{"DAI Sel Mux", "LeftRight", "Amp Enable"},
	{"BE_OUT", NULL, "DAI Sel Mux"},
};

static struct snd_soc_dai_driver max98520_dai[] = {
	{
		.name = "max98520-aif1",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98520_RATES,
			.formats = MAX98520_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98520_RATES,
			.formats = MAX98520_RATES,
		},
		.ops = &max98520_dai_ops,
	}

};

static void max98520_reset(struct max98520_priv *max98520, struct device *dev)
{
	int ret, reg, count;

	pr_info("[MAX98520_DEBUG] %s in", __func__);
	/* Software Reset */
	ret = regmap_write(max98520->regmap,
		MAX98520_R2000_SW_RESET, 1);
	if (ret)
		dev_err(dev, "Reset command failed. (ret:%d)\n", ret);

	count = 0;
	while (count < 3) {
		usleep_range(10000, 11000);
		/* Software Reset Verification */
		ret = regmap_read(max98520->regmap,
			MAX98520_R21FF_REVISION_ID, &reg);
		if (!ret) {
			dev_info(dev, "Reset completed (retry:%d)\n", count);
			return;
		}
		count++;
	}
	dev_err(dev, "Reset failed. (ret:%d)\n", ret);
}

static int max98520_probe(struct snd_soc_component *component)
{
	struct max98520_priv *max98520 =
		snd_soc_component_get_drvdata(component);

	pr_info("[MAX98520_DEBUG] %s in", __func__);
	/* Software Reset */
	max98520_reset(max98520, component->dev);
	usleep_range(30000, 31000);

	/* L/R mix configuration */
	regmap_write(max98520->regmap,
		MAX98520_R2043_PCM_RX_SRC1, 0x2);

	regmap_write(max98520->regmap,
		MAX98520_R2044_PCM_RX_SRC2, 0x10);
	/* Enable DC blocker */
	regmap_update_bits(max98520->regmap,
		MAX98520_R2092_AMP_DSP_CFG, 1, 1);
	/* Disable Speaker Safe Mode */
	regmap_update_bits(max98520->regmap,
		MAX98520_R2092_AMP_DSP_CFG, MAX98520_SPK_SAFE_EN_MASK, 0);
	/* Enable Clock Monitor Auto-restart */
	regmap_write(max98520->regmap,
		MAX98520_R2030_CLK_MON_CTRL, 0x1);

	/* Hard coded values for the experiments */
	regmap_write(max98520->regmap,
		MAX98520_R21FF_REVISION_ID, 0x54);
	regmap_write(max98520->regmap,
		MAX98520_R21FF_REVISION_ID, 0x4d);
	regmap_write(max98520->regmap,
		MAX98520_R2161_BOOST_TM1, 0x2);
	regmap_write(max98520->regmap,
		MAX98520_R2095_AMP_CFG, 0xc8);

	pr_info("[MAX98520_DEBUG] %s out", __func__);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int max98520_suspend(struct device *dev)
{
	struct max98520_priv *max98520 = dev_get_drvdata(dev);

	regcache_cache_only(max98520->regmap, true);
	regcache_mark_dirty(max98520->regmap);
	return 0;
}
static int max98520_resume(struct device *dev)
{
	struct max98520_priv *max98520 = dev_get_drvdata(dev);

	regcache_cache_only(max98520->regmap, false);
	max98520_reset(max98520, dev);
	regcache_sync(max98520->regmap);
	return 0;
}
#endif

static const struct dev_pm_ops max98520_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(max98520_suspend, max98520_resume)
};

static const struct snd_soc_component_driver soc_codec_dev_max98520 = {
	.probe			= max98520_probe,
	.controls		= max98520_snd_controls,
	.num_controls		= ARRAY_SIZE(max98520_snd_controls),
	.dapm_widgets		= max98520_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(max98520_dapm_widgets),
	.dapm_routes		= max98520_audio_map,
	.num_dapm_routes	= ARRAY_SIZE(max98520_audio_map),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static const struct regmap_config max98520_regmap = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = MAX98520_R21FF_REVISION_ID,
	.reg_defaults  = max98520_reg,
	.num_reg_defaults = ARRAY_SIZE(max98520_reg),
	.readable_reg = max98520_readable_register,
	.volatile_reg = max98520_volatile_reg,
	.cache_type = REGCACHE_RBTREE,
};

static int max98520_i2c_probe(struct i2c_client *i2c,
	const struct i2c_device_id *id)
{

	int ret = 0;
	int reg = 0;
	struct max98520_priv *max98520 = NULL;

	pr_info("[MAX98520_DEBUG] %s in", __func__);
	max98520 = devm_kzalloc(&i2c->dev, sizeof(*max98520), GFP_KERNEL);

	if (!max98520) {
		ret = -ENOMEM;
		return ret;
	}
	i2c_set_clientdata(i2c, max98520);

	/* regmap initialization */
	max98520->regmap
		= devm_regmap_init_i2c(i2c, &max98520_regmap);
	if (IS_ERR(max98520->regmap)) {
		ret = PTR_ERR(max98520->regmap);
		dev_err(&i2c->dev,
			"Failed to allocate regmap: %d\n", ret);
		return ret;
	}

	/* Power on device */
	if (gpio_is_valid(max98520->reset_gpio)) {
		ret = devm_gpio_request(&i2c->dev, max98520->reset_gpio,
					"MAX98520_RESET");
		if (ret) {
			dev_err(&i2c->dev, "%s: Failed to request gpio %d\n",
				__func__, max98520->reset_gpio);
			return -EINVAL;
		}
		gpio_direction_output(max98520->reset_gpio, 0);
		msleep(50);
		gpio_direction_output(max98520->reset_gpio, 1);
		msleep(20);
	}

	/* Check Revision ID */
	ret = regmap_read(max98520->regmap,
		MAX98520_R21FF_REVISION_ID, &reg);
	if (ret < 0) {
		dev_err(&i2c->dev,
			"Failed to read: 0x%02X\n", MAX98520_R21FF_REVISION_ID);
		return ret;
	}
	dev_info(&i2c->dev, "MAX98520 revisionID: 0x%02X\n", reg);

	/* codec registration */
	ret = devm_snd_soc_register_component(&i2c->dev,
					      &soc_codec_dev_max98520,
		max98520_dai, ARRAY_SIZE(max98520_dai));
	if (ret < 0)
		dev_err(&i2c->dev, "Failed to register codec: %d\n", ret);

	pr_info("[MAX98520_DEBUG] %s out", __func__);
	return ret;
}

static const struct i2c_device_id max98520_i2c_id[] = {
	{ "max98520", 0},
	{ },
};

MODULE_DEVICE_TABLE(i2c, max98520_i2c_id);

#if defined(CONFIG_OF)
static const struct of_device_id max98520_of_match[] = {
	{ .compatible = "maxim,max98520", },
	{ }
};
MODULE_DEVICE_TABLE(of, max98520_of_match);
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id max98520_acpi_match[] = {
	{ "MX98520", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, max98520_acpi_match);
#endif

static struct i2c_driver max98520_i2c_driver = {
	.driver = {
		.name = "max98520",
		.of_match_table = of_match_ptr(max98520_of_match),
		.acpi_match_table = ACPI_PTR(max98520_acpi_match),
		.pm = &max98520_pm,
	},
	.probe = max98520_i2c_probe,
	.id_table = max98520_i2c_id,
};

module_i2c_driver(max98520_i2c_driver)

MODULE_DESCRIPTION("ALSA SoC MAX98520 driver");
MODULE_AUTHOR("Ryan Lee <ryans.lee@maximintegrated.com>");
MODULE_LICENSE("GPL");
