// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * max98373.c  --  MAX98373 ALSA Soc Audio driver
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

#include "../codecs/max98373.h"

static const unsigned int bcm2835_rates_12000000[] = {
	8000, 16000, 32000, 44100, 48000, 96000, 88200,
};

static struct snd_pcm_hw_constraint_list bcm2835_constraints_12000000 = {
	.list = bcm2835_rates_12000000,
	.count = ARRAY_SIZE(bcm2835_rates_12000000),
};

static int snd_audioinjector_max98373_soundcard_startup
	(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;

	pr_info("[GEORGE] %s\n", __func__);
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

static int snd_audioinjector_max98373_soundcard_hw_params
	(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int ret = 0;
	int sr = params_rate(params);

	pr_info("[GEORGE] %s sample rate = %d\n", __func__, sr);

	// set codec DAI configuration
	ret = snd_soc_dai_set_fmt(rtd->codec_dai,
			SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_I2S|
			SND_SOC_DAIFMT_NB_NF);
	if (ret < 0)
		return ret;

	pr_info("[GEORGE] %s codec ret : %d\n", __func__, ret);

	// set cpu DAI configuration
	ret = snd_soc_dai_set_fmt(rtd->cpu_dai,
			SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_I2S|
			SND_SOC_DAIFMT_NB_NF);
	if (ret < 0)
		return ret;

	pr_info("[GEORGE] %s cpu ret : %d\n", __func__, ret);
	if (ret < 0)
		return ret;

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
	pr_info("[GEORGE] %s out. ret:%d\n", __func__, ret);
	return ret;
}

/* machine stream operations */
static struct snd_soc_ops snd_audioinjector_max98373_soundcard_ops = {
	.startup = snd_audioinjector_max98373_soundcard_startup,
	.hw_params = snd_audioinjector_max98373_soundcard_hw_params,
};

static int audioinjector_max98373_soundcard_dai_init
	(struct snd_soc_pcm_runtime *rtd)
{
	pr_info("[GEORGE] %s\n", __func__);

	return 0;
}

SND_SOC_DAILINK_DEFS(audioinjector_max98373,
	DAILINK_COMP_ARRAY(COMP_CPU("bcm2708-i2s.0")),
	DAILINK_COMP_ARRAY(COMP_CODEC("max98373.1-0038", "max98373-aif1")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("bcm2835-i2s.0")));

static struct snd_soc_dai_link audioinjector_max98373_soundcard_dai[] = {
	{
		.name = "AudioInjector audio",
		.stream_name = "AudioInjector audio",
		.ops = &snd_audioinjector_max98373_soundcard_ops,
		.init = audioinjector_max98373_soundcard_dai_init,
		.dai_fmt = SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_I2S
			   | SND_SOC_DAIFMT_NB_NF,
		SND_SOC_DAILINK_REG(audioinjector_max98373),
	},
};

static const struct snd_soc_dapm_widget max98373_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
	SND_SOC_DAPM_MIC("Microphone", NULL),
};

static const struct snd_soc_dapm_route audioinjector_audio_map[] = {
	/* speaker connected to LOUT, ROUT */
	{"Ext Spk", NULL, "BE_OUT"},

	/* mic is connected to Mic Jack, with max98373 Mic Bias */
//	{"Microphone", NULL, "Mic Bias"},
};

static struct snd_soc_card snd_soc_audioinjector = {
	.name = "audioinjector-max98373-soundcard",
	.dai_link = audioinjector_max98373_soundcard_dai,
	.num_links = ARRAY_SIZE(audioinjector_max98373_soundcard_dai),

	.dapm_widgets = max98373_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(max98373_dapm_widgets),
	.dapm_routes = audioinjector_audio_map,
	.num_dapm_routes = ARRAY_SIZE(audioinjector_audio_map),
};

static int audioinjector_max98373_soundcard_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &snd_soc_audioinjector;
	int ret;

	pr_info("[GEORGE] %s +++ \n", __func__);
	card->dev = &pdev->dev;

	if (pdev->dev.of_node) {
		struct snd_soc_dai_link *dai =
			&audioinjector_max98373_soundcard_dai[0];
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
	pr_info("[GEORGE] %s --- ret : %d\n", __func__, ret);
	return ret;
}

static const struct of_device_id audioinjector_max98373_soundcard_of_match[] = {
	{ .compatible = "ai,audioinjector-max98373-soundcard", },
	{},
};
MODULE_DEVICE_TABLE(of, audioinjector_max98373_soundcard_of_match);

static struct platform_driver audioinjector_max98373_soundcard_driver = {
	.driver = {
		.name = "audioinjector-max98373",
		.owner = THIS_MODULE,
		.of_match_table = audioinjector_max98373_soundcard_of_match,
	},
	.probe = audioinjector_max98373_soundcard_probe,
};

module_platform_driver(audioinjector_max98373_soundcard_driver);
MODULE_AUTHOR("Matt Flax <flatmax@flatmax.org>");
MODULE_DESCRIPTION("AudioInjector.net Pi Soundcard");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:audioinjector-pi-soundcard");

