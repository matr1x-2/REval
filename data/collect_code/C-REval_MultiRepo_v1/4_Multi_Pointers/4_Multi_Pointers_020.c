/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_020 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 10 */
/* NLOC: 54 */
/* Subsystem: net */
/* Includes
 * #include <linux/module.h>
 * #include <linux/openvswitch.h>
 * #include <linux/tcp.h>
 * #include <linux/udp.h>
 * #include <linux/sctp.h>
 * #include <linux/static_key.h>
 * #include <linux/string_helpers.h>
 * #include <net/ip.h>
 * #include <net/genetlink.h>
 * #include <net/netfilter/nf_conntrack_core.h>
 * #include <net/netfilter/nf_conntrack_count.h>
 * #include <net/netfilter/nf_conntrack_helper.h>
 * #include <net/netfilter/nf_conntrack_labels.h>
 * #include <net/netfilter/nf_conntrack_seqadj.h>
 * #include <net/netfilter/nf_conntrack_timeout.h>
 * #include <net/netfilter/nf_conntrack_zones.h>
 * #include <net/netfilter/ipv6/nf_defrag_ipv6.h>
 * #include <net/ipv6_frag.h>
 * #include <net/netfilter/nf_nat.h>
 * #include <net/netfilter/nf_conntrack_act_ct.h>
 */
/* Context-Before
 * bool ovs_ct_verify(struct net *net, enum ovs_key_attr attr)
 * {
 * 	if (attr == OVS_KEY_ATTR_CT_STATE)
 * 		return true;
 * 	if (IS_ENABLED(CONFIG_NF_CONNTRACK_ZONES) &&
 * 	    attr == OVS_KEY_ATTR_CT_ZONE)
 * 		return true;
 * 	if (IS_ENABLED(CONFIG_NF_CONNTRACK_MARK) &&
 * 	    attr == OVS_KEY_ATTR_CT_MARK)
 * 		return true;
 * 	if (IS_ENABLED(CONFIG_NF_CONNTRACK_LABELS) &&
 * 	    attr == OVS_KEY_ATTR_CT_LABELS) {
 * 		struct ovs_net *ovs_net = net_generic(net, ovs_net_id);
 * 
 * 		return ovs_net->xt_label;
 * 	}
 * 
 * 	return false;
 * }
 */
int ovs_ct_copy_action(struct net *net, const struct nlattr *attr,
		       const struct sw_flow_key *key,
		       struct sw_flow_actions **sfa,  bool log)
{
	struct ovs_conntrack_info ct_info;
	const char *helper = NULL;
	u16 family;
	int err;

	family = key_to_nfproto(key);
	if (family == NFPROTO_UNSPEC) {
		OVS_NLERR(log, "ct family unspecified");
		return -EINVAL;
	}

	memset(&ct_info, 0, sizeof(ct_info));
	ct_info.family = family;

	nf_ct_zone_init(&ct_info.zone, NF_CT_DEFAULT_ZONE_ID,
			NF_CT_DEFAULT_ZONE_DIR, 0);

	err = parse_ct(attr, &ct_info, &helper, log);
	if (err)
		return err;

	/* Set up template for tracking connections in specific zones. */
	ct_info.ct = nf_ct_tmpl_alloc(net, &ct_info.zone, GFP_KERNEL);
	if (!ct_info.ct) {
		OVS_NLERR(log, "Failed to allocate conntrack template");
		return -ENOMEM;
	}

	if (ct_info.timeout[0]) {
		if (nf_ct_set_timeout(net, ct_info.ct, family, key->ip.proto,
				      ct_info.timeout))
			OVS_NLERR(log,
				  "Failed to associated timeout policy '%s'",
				  ct_info.timeout);
		else
			ct_info.nf_ct_timeout = rcu_dereference(
				nf_ct_timeout_find(ct_info.ct)->timeout);

	}

	if (helper) {
		err = nf_ct_add_helper(ct_info.ct, helper, ct_info.family,
				       key->ip.proto, ct_info.nat, &ct_info.helper);
		if (err) {
			OVS_NLERR(log, "Failed to add %s helper %d", helper, err);
			goto err_free_ct;
		}
	}

	err = ovs_nla_add_action(sfa, OVS_ACTION_ATTR_CT, &ct_info,
				 sizeof(ct_info), log);
	if (err)
		goto err_free_ct;

	if (ct_info.commit)
		__set_bit(IPS_CONFIRMED_BIT, &ct_info.ct->status);
	return 0;
err_free_ct:
	__ovs_ct_free_action(&ct_info);
	return err;
}
/* Context-After
 * #if IS_ENABLED(CONFIG_NF_NAT)
 * static bool ovs_ct_nat_to_attr(const struct ovs_conntrack_info *info,
 * 			       struct sk_buff *skb)
 * {
 * 	struct nlattr *start;
 * 
 * 	start = nla_nest_start_noflag(skb, OVS_CT_ATTR_NAT);
 * 	if (!start)
 * 		return false;
 * 
 * 	if (info->nat & OVS_CT_SRC_NAT) {
 * 		if (nla_put_flag(skb, OVS_NAT_ATTR_SRC))
 * 			return false;
 * 	} else if (info->nat & OVS_CT_DST_NAT) {
 * 		if (nla_put_flag(skb, OVS_NAT_ATTR_DST))
 * 			return false;
 * 	} else {
 * 		goto out;
 * 	}
 */
