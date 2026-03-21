/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_036 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 5 */
/* NLOC: 30 */
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
 * #define graph_ret(priv, ret) _graph_ret(priv, __func__, ret)
 * static inline int _graph_ret(struct simple_util_priv *priv,
 * 			       const char *func, int ret)
 * {
 * 	return snd_soc_ret(simple_priv_to_dev(priv), ret, "at %s()\n", func);
 * }
 * 
 * #define ep_to_port(ep)	of_get_parent(ep)
 * static struct device_node *port_to_ports(struct device_node *port)
 * {
 * 	struct device_node *ports = of_get_parent(port);
 * 
 * 	if (!of_node_name_eq(ports, "ports")) {
 * 		of_node_put(ports);
 * 		return NULL;
 * 	}
 * 	return ports;
 * }
 */
static enum graph_type __graph_get_type(struct device_node *lnk)
{
	struct device_node *np, *parent_np;
	enum graph_type ret;

	/*
	 * target {
	 *	ports {
	 * =>		lnk:	port@0 { ... };
	 *			port@1 { ... };
	 *	};
	 * };
	 */
	np = of_get_parent(lnk);
	if (of_node_name_eq(np, "ports")) {
		parent_np = of_get_parent(np);
		of_node_put(np);
		np = parent_np;
	}

	if (of_node_name_eq(np, GRAPH_NODENAME_MULTI)) {
		ret = GRAPH_MULTI;
		fw_devlink_purge_absent_suppliers(&np->fwnode);
		goto out_put;
	}

	if (of_node_name_eq(np, GRAPH_NODENAME_DPCM)) {
		ret = GRAPH_DPCM;
		fw_devlink_purge_absent_suppliers(&np->fwnode);
		goto out_put;
	}

	if (of_node_name_eq(np, GRAPH_NODENAME_C2C)) {
		ret = GRAPH_C2C;
		fw_devlink_purge_absent_suppliers(&np->fwnode);
		goto out_put;
	}

	ret = GRAPH_NORMAL;

out_put:
	of_node_put(np);
	return ret;

}
/* Context-After
 * static enum graph_type graph_get_type(struct simple_util_priv *priv,
 * 				      struct device_node *lnk)
 * {
 * 	enum graph_type type = __graph_get_type(lnk);
 * 
 * 	/* GRAPH_MULTI here means GRAPH_NORMAL */
 * 	if (type == GRAPH_MULTI)
 * 		type = GRAPH_NORMAL;
 * 
 * #ifdef DEBUG
 * 	{
 * 		struct device *dev = simple_priv_to_dev(priv);
 * 		const char *str = "Normal";
 * 
 * 		switch (type) {
 * 		case GRAPH_DPCM:
 * 			if (graph_util_is_ports0(lnk))
 * 				str = "DPCM Front-End";
 * 			else
 */
