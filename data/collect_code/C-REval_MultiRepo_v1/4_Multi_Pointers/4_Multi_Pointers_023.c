/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_023 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 89 */
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
 * 	 * This function is called by (C) -> (B) -> (A) order.
 * 	 * Set if applicable part was not yet set.
 * 	 */
 * 	fmt = snd_soc_daifmt_parse_format(node, NULL);
 * 	update_daifmt(FORMAT);
 * 	update_daifmt(CLOCK);
 * 	update_daifmt(INV);
 * }
 * 
 * static unsigned int graph_parse_bitframe(struct device_node *ep)
 * {
 * 	struct device_node *port  __free(device_node) = ep_to_port(ep);
 * 	struct device_node *ports __free(device_node) = port_to_ports(port);
 * 
 * 	return	snd_soc_daifmt_clock_provider_from_bitmap(
 * 			snd_soc_daifmt_parse_clock_provider_as_bitmap(ep,    NULL) |
 * 			snd_soc_daifmt_parse_clock_provider_as_bitmap(port,  NULL) |
 * 			snd_soc_daifmt_parse_clock_provider_as_bitmap(ports, NULL));
 * }
 */
static void graph_link_init(struct simple_util_priv *priv,
			    struct device_node *lnk,
			    struct device_node *ep_cpu,
			    struct device_node *ep_codec,
			    struct link_info *li,
			    int is_cpu_node)
{
	struct snd_soc_dai_link *dai_link = simple_priv_to_link(priv, li->link);
	struct simple_dai_props *dai_props = simple_priv_to_props(priv, li->link);
	struct device_node *port_cpu = ep_to_port(ep_cpu);
	struct device_node *port_codec = ep_to_port(ep_codec);
	struct device_node *multi_cpu_port = NULL, *multi_codec_port = NULL;
	struct snd_soc_dai_link_component *dlc;
	unsigned int daifmt = 0;
	bool playback_only = 0, capture_only = 0;
	enum snd_soc_trigger_order trigger_start = SND_SOC_TRIGGER_ORDER_DEFAULT;
	enum snd_soc_trigger_order trigger_stop  = SND_SOC_TRIGGER_ORDER_DEFAULT;
	int multi_cpu_port_idx = 1, multi_codec_port_idx = 1;
	int i;

	if (graph_lnk_is_multi(port_cpu)) {
		multi_cpu_port = port_cpu;
		ep_cpu = graph_get_next_multi_ep(&multi_cpu_port, multi_cpu_port_idx++);
		of_node_put(port_cpu);
		port_cpu = ep_to_port(ep_cpu);
	} else {
		of_node_get(ep_cpu);
	}
	struct device_node *ports_cpu __free(device_node) = port_to_ports(port_cpu);

	if (graph_lnk_is_multi(port_codec)) {
		multi_codec_port = port_codec;
		ep_codec = graph_get_next_multi_ep(&multi_codec_port, multi_codec_port_idx++);
		of_node_put(port_codec);
		port_codec = ep_to_port(ep_codec);
	} else {
		of_node_get(ep_codec);
	}
	struct device_node *ports_codec __free(device_node) = port_to_ports(port_codec);

	graph_parse_daifmt(ep_cpu,	&daifmt);
	graph_parse_daifmt(ep_codec,	&daifmt);
	graph_parse_daifmt(port_cpu,	&daifmt);
	graph_parse_daifmt(port_codec,	&daifmt);
	graph_parse_daifmt(ports_cpu,	&daifmt);
	graph_parse_daifmt(ports_codec,	&daifmt);
	graph_parse_daifmt(lnk,		&daifmt);

	graph_util_parse_link_direction(lnk,		&playback_only, &capture_only);
	graph_util_parse_link_direction(ports_cpu,	&playback_only, &capture_only);
	graph_util_parse_link_direction(ports_codec,	&playback_only, &capture_only);
	graph_util_parse_link_direction(port_cpu,	&playback_only, &capture_only);
	graph_util_parse_link_direction(port_codec,	&playback_only, &capture_only);
	graph_util_parse_link_direction(ep_cpu,		&playback_only, &capture_only);
	graph_util_parse_link_direction(ep_codec,	&playback_only, &capture_only);

	of_property_read_u32(lnk,		"mclk-fs", &dai_props->mclk_fs);
	of_property_read_u32(ports_cpu,		"mclk-fs", &dai_props->mclk_fs);
	of_property_read_u32(ports_codec,	"mclk-fs", &dai_props->mclk_fs);
	of_property_read_u32(port_cpu,		"mclk-fs", &dai_props->mclk_fs);
	of_property_read_u32(port_codec,	"mclk-fs", &dai_props->mclk_fs);
	of_property_read_u32(ep_cpu,		"mclk-fs", &dai_props->mclk_fs);
	of_property_read_u32(ep_codec,		"mclk-fs", &dai_props->mclk_fs);

	graph_util_parse_trigger_order(priv, lnk,		&trigger_start, &trigger_stop);
	graph_util_parse_trigger_order(priv, ports_cpu,		&trigger_start, &trigger_stop);
	graph_util_parse_trigger_order(priv, ports_codec,	&trigger_start, &trigger_stop);
	graph_util_parse_trigger_order(priv, port_cpu,		&trigger_start, &trigger_stop);
	graph_util_parse_trigger_order(priv, port_cpu,		&trigger_start, &trigger_stop);
	graph_util_parse_trigger_order(priv, ep_cpu,		&trigger_start, &trigger_stop);
	graph_util_parse_trigger_order(priv, ep_codec,		&trigger_start, &trigger_stop);

	for_each_link_cpus(dai_link, i, dlc) {
		dlc->ext_fmt = graph_parse_bitframe(ep_cpu);

		if (multi_cpu_port)
			ep_cpu = graph_get_next_multi_ep(&multi_cpu_port, multi_cpu_port_idx++);
	}

	for_each_link_codecs(dai_link, i, dlc) {
		dlc->ext_fmt = graph_parse_bitframe(ep_codec);

		if (multi_codec_port)
			ep_codec = graph_get_next_multi_ep(&multi_codec_port, multi_codec_port_idx++);
	}

	/*** Don't use port_cpu / port_codec after here ***/

	dai_link->playback_only	= playback_only;
	dai_link->capture_only	= capture_only;

	dai_link->trigger_start	= trigger_start;
	dai_link->trigger_stop	= trigger_stop;

	dai_link->dai_fmt	= daifmt;
	dai_link->init		= simple_util_dai_init;
	dai_link->ops		= &graph_ops;
	if (priv->ops)
		dai_link->ops	= priv->ops;

	of_node_put(port_cpu);
	of_node_put(port_codec);
	of_node_put(ep_cpu);
	of_node_put(ep_codec);
}
/* Context-After
 * int audio_graph2_link_normal(struct simple_util_priv *priv,
 * 			     struct device_node *lnk,
 * 			     struct link_info *li)
 * {
 * 	struct device_node *cpu_port = lnk;
 * 	struct device_node *cpu_ep	__free(device_node) = of_graph_get_next_port_endpoint(cpu_port, NULL);
 * 	struct device_node *codec_ep	__free(device_node) = of_graph_get_remote_endpoint(cpu_ep);
 * 	int ret;
 * 
 * 	/*
 * 	 * call Codec first.
 * 	 * see
 * 	 *	__graph_parse_node() :: DAI Naming
 * 	 */
 * 	ret = graph_parse_node(priv, GRAPH_NORMAL, codec_ep, li, 0);
 * 	if (ret < 0)
 * 		goto end;
 * 
 * 	/*
 */
