// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * max98390.c  --  MAX98390 ALSA Soc Audio driver
 *
 * Copyright (C) 2020 Maxim Integrated Products
 *
 */

#include <linux/module.h>
#include <linux/types.h>

#include <sound/core.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/control.h>

#include "../codecs/max98396.h"

#define USE_TDM_MODE

#ifdef USE_TDM_MODE
#define DAI_FMT_BASE (SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_NB_NF)
#else
#define DAI_FMT_BASE (SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF)	
#endif

static const unsigned int bcm2835_rates_12000000[] = {
	8000, 16000, 32000, 44100, 48000, 96000, 88200,
};

static struct snd_pcm_hw_constraint_list bcm2835_constraints_12000000 = {
	.list = bcm2835_rates_12000000,
	.count = ARRAY_SIZE(bcm2835_rates_12000000),
};

static int snd_audioinjector_maxim_soundcard_startup
	(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;

	pr_info("[RYAN] %s\n", __func__);
	rtd->cpu_dai->driver->playback.channels_min = 2;
	rtd->cpu_dai->driver->playback.channels_max = 2;
	rtd->cpu_dai->driver->capture.channels_min = 2;
	rtd->cpu_dai->driver->capture.channels_max = 2;
	rtd->codec_dai->driver->capture.channels_max = 2;

	/* Setup constraints, because there is a 12 MHz XTAL on the board */
	snd_pcm_hw_constraint_list(substream->runtime, 0,
				SNDRV_PCM_HW_PARAM_RATE,
				&bcm2835_constraints_12000000);
	return 0;
}

static int snd_audioinjector_maxim_soundcard_hw_params
	(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int ret = 0;
	int sr = params_rate(params);
	unsigned int daiFmt = 0; 

#ifdef USE_TDM_MODE
	// set TDM slot configuration vmon_slot_no 0 for Tx slots
	int width = params_width(params);
	int ch = params_channels(params);
	
	pr_info("[RYAN] %s sample ch = %d, width = %d\n", __func__, ch, width);
	
	ret = snd_soc_dai_set_tdm_slot(rtd->codec_dai, 1, 0x10, ch, width);
	if (ret < 0)
		return ret;

	pr_info("[RYAN] %s codec tdm slot config ret : %d\n", __func__, ret);

	ret = snd_soc_dai_set_tdm_slot(rtd->cpu_dai, 0x03, 0x03, ch, width);
	if (ret < 0)
		return ret;

	pr_info("[RYAN] %s cpu tdm slot config ret : %d\n", __func__, ret);
	
	daiFmt = SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_DSP_B|SND_SOC_DAIFMT_NB_NF;
#else
	daiFmt = SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_NB_NF;
#endif

	pr_info("[RYAN] %s sample rate = %d\n", __func__, sr);
	
#if 0
	switch (sr){
	case 8000:
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 1);
		break;
	case 16000:
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 750);
		break;
	case 32000:
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 375);
		break;
	case 44100:
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 272);
		break;
	case 48000:
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 250);
		break;
	case 88200:
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 136);
		break;
	case 96000:
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 125);
		break;
	default:
		ret = snd_soc_dai_set_bclk_ratio(cpu_dai, 125);
		break;
	}
#endif
	pr_info("[RYAN] %s out. ret:%d\n", __func__, ret);
	return ret;
}

/* machine stream operations */
static struct snd_soc_ops snd_audioinjector_maxim_soundcard_ops = {
	.startup = snd_audioinjector_maxim_soundcard_startup,
	.hw_params = snd_audioinjector_maxim_soundcard_hw_params,
};

static int audioinjector_maxim_soundcard_dai_init
	(struct snd_soc_pcm_runtime *rtd)
{	
	pr_info("[RYAN] %s\n", __func__);
	return 0;
}

SND_SOC_DAILINK_DEFS(audioinjector_maxim,
	DAILINK_COMP_ARRAY(COMP_CPU("bcm2708-i2s.0")),
	DAILINK_COMP_ARRAY(COMP_CODEC("max98396.1-0038", "max98396-aif1")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("bcm2835-i2s.0")));

static struct snd_soc_dai_link audioinjector_maxim_soundcard_dai[] = {
	{
		.name = "AudioInjector audio",
		.stream_name = "AudioInjector audio",
		.ops = &snd_audioinjector_maxim_soundcard_ops,
		.init = audioinjector_maxim_soundcard_dai_init,
		.dai_fmt = DAI_FMT_BASE,
		SND_SOC_DAILINK_REG(audioinjector_maxim),
	},
};

static const struct snd_soc_dapm_widget max98396_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
	SND_SOC_DAPM_MIC("Microphone", NULL),
};

static const struct snd_soc_dapm_route audioinjector_audio_map[] = {
	/* speaker connected to LOUT, ROUT */
	{"Ext Spk", NULL, "BE_OUT"},

	/* mic is connected to Mic Jack, with max98396 Mic Bias */
//	{"Microphone", NULL, "Mic Bias"},
};

static struct snd_soc_card snd_soc_audioinjector = {
	.name = "audioinjector-maxim-soundcard",
	.dai_link = audioinjector_maxim_soundcard_dai,
	.num_links = ARRAY_SIZE(audioinjector_maxim_soundcard_dai),

	.dapm_widgets = max98396_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(max98396_dapm_widgets),
	.dapm_routes = audioinjector_audio_map,
	.num_dapm_routes = ARRAY_SIZE(audioinjector_audio_map),
};

static int audioinjector_maxim_soundcard_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &snd_soc_audioinjector;
	int ret;

	pr_info("[RYAN] %s +++ \n", __func__);
	card->dev = &pdev->dev;

	if (pdev->dev.of_node) {
		struct snd_soc_dai_link *dai =
			&audioinjector_maxim_soundcard_dai[0];
		struct device_node *i2s_node =
			of_parse_phandle(pdev->dev.of_node,
					 "i2s-controller", 0);

		if (i2s_node) {
			dai->cpus->dai_name = NULL;
			dai->cpus->of_node = i2s_node;
			dai->platforms->name = NULL;
			dai->platforms->of_node = i2s_node;
		} else
			if (!dai->cpus->of_node) {
				dev_err(&pdev->dev, "Property 'i2s-controller' missing or invalid\n");
				return -EINVAL;
			}
	}

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);
	}
	pr_info("[RYAN] %s --- ret : %d\n", __func__, ret);
	return ret;
}

static const struct of_device_id audioinjector_maxim_soundcard_of_match[] = {
	{ .compatible = "ai,audioinjector-maxim-soundcard", },
	{},
};
MODULE_DEVICE_TABLE(of, audioinjector_maxim_soundcard_of_match);

static struct platform_driver audioinjector_maxim_soundcard_driver = {
	.driver = {
		.name = "audioinjector-maxim",
		.owner = THIS_MODULE,
		.of_match_table = audioinjector_maxim_soundcard_of_match,
	},
	.probe = audioinjector_maxim_soundcard_probe,
};

module_platform_driver(audioinjector_maxim_soundcard_driver);
MODULE_AUTHOR("Matt Flax <flatmax@flatmax.org>");
MODULE_DESCRIPTION("AudioInjector.net Pi Soundcard");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:audioinjector-pi-soundcard");

