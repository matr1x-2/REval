/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_042 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 18 */
/* NLOC: 82 */
/* Subsystem: sound */
/* Includes
 * #include <linux/clk.h>
 * #include <linux/device.h>
 * #include <linux/gpio/consumer.h>
 * #include <linux/module.h>
 * #include <linux/of.h>
 * #include <linux/of_graph.h>
 * #include <linux/platform_device.h>
 * #include <linux/string.h>
 * #include <sound/graph_card.h>
 */
/* Context-Before
 * }
 * 
 * static const struct snd_soc_ops graph_ops = {
 * 	.startup	= simple_util_startup,
 * 	.shutdown	= simple_util_shutdown,
 * 	.hw_params	= simple_util_hw_params,
 * };
 * 
 * static void graph_parse_convert(struct device_node *ep,
 * 				struct simple_dai_props *props)
 * {
 * 	struct device_node *port  __free(device_node) = ep_to_port(ep);
 * 	struct device_node *ports __free(device_node) = port_to_ports(port);
 * 	struct simple_util_data *adata = &props->adata;
 * 
 * 	simple_util_parse_convert(ports, NULL, adata);
 * 	simple_util_parse_convert(port, NULL, adata);
 * 	simple_util_parse_convert(ep,   NULL, adata);
 * }
 */
static int __graph_parse_node(struct simple_util_priv *priv,
			      enum graph_type gtype,
			      struct device_node *ep,
			      struct link_info *li,
			      int is_cpu, int idx)
{
	struct device *dev = simple_priv_to_dev(priv);
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, li->link);
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, li->link);
	struct snd_soc_dai_link_component *dlc;
	struct simple_util_dai *dai;
	int ret, is_single_links = 0;

	if (is_cpu) {
		dlc = snd_soc_link_to_cpu(dai_link, idx);
		dai = simple_props_to_dai_cpu(dai_props, idx);
	} else {
		dlc = snd_soc_link_to_codec(dai_link, idx);
		dai = simple_props_to_dai_codec(dai_props, idx);
	}

	ret = graph_util_parse_dai(priv, ep, dlc, &is_single_links);
	if (ret < 0)
		goto end;

	ret = simple_util_parse_tdm(ep, dai);
	if (ret < 0)
		goto end;

	ret = simple_util_parse_tdm_width_map(priv, ep, dai);
	if (ret < 0)
		goto end;

	ret = simple_util_parse_clk(dev, ep, dai, dlc);
	if (ret < 0)
		goto end;

	/*
	 * set DAI Name
	 */
	if (!dai_link->name) {
		struct snd_soc_dai_link_component *cpus = dlc;
		struct snd_soc_dai_link_component *codecs = snd_soc_link_to_codec(dai_link, idx);
		char *cpu_multi   = "";
		char *codec_multi = "";

		if (dai_link->num_cpus > 1)
			cpu_multi = "_multi";
		if (dai_link->num_codecs > 1)
			codec_multi = "_multi";

		switch (gtype) {
		case GRAPH_NORMAL:
			/* run is_cpu only. see audio_graph2_link_normal() */
			if (is_cpu)
				simple_util_set_dailink_name(priv, dai_link, "%s%s-%s%s",
							       cpus->dai_name,   cpu_multi,
							     codecs->dai_name, codec_multi);
			break;
		case GRAPH_DPCM:
			if (is_cpu)
				simple_util_set_dailink_name(priv, dai_link, "fe.%pOFP.%s%s",
						cpus->of_node, cpus->dai_name, cpu_multi);
			else
				simple_util_set_dailink_name(priv, dai_link, "be.%pOFP.%s%s",
						codecs->of_node, codecs->dai_name, codec_multi);
			break;
		case GRAPH_C2C:
			/* run is_cpu only. see audio_graph2_link_c2c() */
			if (is_cpu)
				simple_util_set_dailink_name(priv, dai_link, "c2c.%s%s-%s%s",
							     cpus->dai_name,   cpu_multi,
							     codecs->dai_name, codec_multi);
			break;
		default:
			break;
		}
	}

	/*
	 * Check "prefix" from top node
	 * if DPCM-BE case
	 */
	if (!is_cpu && gtype == GRAPH_DPCM) {
		struct snd_soc_dai_link_component *codecs = snd_soc_link_to_codec(dai_link, idx);
		struct snd_soc_codec_conf *cconf = simple_props_to_codec_conf(dai_props, idx);
		struct device_node *rport  __free(device_node) = ep_to_port(ep);
		struct device_node *rports __free(device_node) = port_to_ports(rport);

		snd_soc_of_parse_node_prefix(rports, cconf, codecs->of_node, "prefix");
		snd_soc_of_parse_node_prefix(rport,  cconf, codecs->of_node, "prefix");
	}

	if (is_cpu) {
		struct snd_soc_dai_link_component *cpus = dlc;
		struct snd_soc_dai_link_component *platforms = snd_soc_link_to_platform(dai_link, idx);

		simple_util_canonicalize_cpu(cpus, is_single_links);
		simple_util_canonicalize_platform(platforms, cpus);
	}
end:
	return graph_ret(priv, ret);
}
/* Context-After
 * static int graph_parse_node_multi_nm(struct simple_util_priv *priv,
 * 				     struct snd_soc_dai_link *dai_link,
 * 				     int *nm_idx, int cpu_idx,
 * 				     struct device_node *mcpu_port)
 * {
 * 	/*
 * 	 *		+---+		+---+
 * 	 *		|  X|<-@------->|x  |
 * 	 *		|   |		|   |
 * 	 *	cpu0 <--|A 1|<--------->|4 a|-> codec0
 * 	 *	cpu1 <--|B 2|<-----+--->|5 b|-> codec1
 * 	 *	cpu2 <--|C 3|<----/	+---+
 * 	 *		+---+
 * 	 *
 * 	 * multi {
 * 	 *	ports {
 * 	 *		port@0 { mcpu_top_ep	{...  = mcodec_ep;	}; };	// (X) to pair
 * 	 * <mcpu_port>	port@1 { mcpu0_ep	{ ... = cpu0_ep;	};	// (A) Multi Element
 * 	 *			 mcpu0_ep_0	{ ... = mcodec0_ep_0;	}; };	// (1) connected Codec
 */
