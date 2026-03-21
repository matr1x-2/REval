/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_036 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 43 */
/* NLOC: 226 */
/* Subsystem: net */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/init.h>
 * #include <linux/module.h>
 * #include <linux/rhashtable.h>
 * #include <linux/workqueue.h>
 * #include <linux/refcount.h>
 * #include <linux/bitfield.h>
 * #include <linux/if_ether.h>
 * #include <linux/in6.h>
 * #include <linux/ip.h>
 * #include <linux/mpls.h>
 * #include <linux/ppp_defs.h>
 * #include <net/sch_generic.h>
 * #include <net/pkt_cls.h>
 * #include <net/pkt_sched.h>
 * #include <net/ip.h>
 * #include <net/flow_dissector.h>
 * #include <net/geneve.h>
 * #include <net/vxlan.h>
 * #include <net/erspan.h>
 */
/* Context-Before
 * 			  struct fl_flow_key *mask,
 * 			  struct netlink_ext_ack *extack)
 * {
 * 	struct nlattr *nla_cfm_opt[TCA_FLOWER_KEY_CFM_OPT_MAX + 1];
 * 	int err;
 * 
 * 	if (!tb[TCA_FLOWER_KEY_CFM])
 * 		return 0;
 * 
 * 	err = nla_parse_nested(nla_cfm_opt, TCA_FLOWER_KEY_CFM_OPT_MAX,
 * 			       tb[TCA_FLOWER_KEY_CFM], cfm_opt_policy, extack);
 * 	if (err < 0)
 * 		return err;
 * 
 * 	fl_set_key_cfm_opcode(nla_cfm_opt, key, mask, extack);
 * 	fl_set_key_cfm_md_level(nla_cfm_opt, key, mask, extack);
 * 
 * 	return 0;
 * }
 */
static int fl_set_key(struct net *net, struct nlattr *tca_opts,
		      struct nlattr **tb, struct fl_flow_key *key,
		      struct fl_flow_key *mask, struct netlink_ext_ack *extack)
{
	__be16 ethertype;
	int ret = 0;

	if (tb[TCA_FLOWER_INDEV]) {
		int err = tcf_change_indev(net, tb[TCA_FLOWER_INDEV], extack);
		if (err < 0)
			return err;
		key->meta.ingress_ifindex = err;
		mask->meta.ingress_ifindex = 0xffffffff;
	}

	fl_set_key_val(tb, &key->meta.l2_miss, TCA_FLOWER_L2_MISS,
		       &mask->meta.l2_miss, TCA_FLOWER_UNSPEC,
		       sizeof(key->meta.l2_miss));

	fl_set_key_val(tb, key->eth.dst, TCA_FLOWER_KEY_ETH_DST,
		       mask->eth.dst, TCA_FLOWER_KEY_ETH_DST_MASK,
		       sizeof(key->eth.dst));
	fl_set_key_val(tb, key->eth.src, TCA_FLOWER_KEY_ETH_SRC,
		       mask->eth.src, TCA_FLOWER_KEY_ETH_SRC_MASK,
		       sizeof(key->eth.src));
	fl_set_key_val(tb, &key->num_of_vlans,
		       TCA_FLOWER_KEY_NUM_OF_VLANS,
		       &mask->num_of_vlans,
		       TCA_FLOWER_UNSPEC,
		       sizeof(key->num_of_vlans));

	if (is_vlan_key(tb[TCA_FLOWER_KEY_ETH_TYPE], &ethertype, key, mask, 0)) {
		fl_set_key_vlan(tb, ethertype, TCA_FLOWER_KEY_VLAN_ID,
				TCA_FLOWER_KEY_VLAN_PRIO,
				TCA_FLOWER_KEY_VLAN_ETH_TYPE,
				&key->vlan, &mask->vlan);

		if (is_vlan_key(tb[TCA_FLOWER_KEY_VLAN_ETH_TYPE],
				&ethertype, key, mask, 1)) {
			fl_set_key_vlan(tb, ethertype,
					TCA_FLOWER_KEY_CVLAN_ID,
					TCA_FLOWER_KEY_CVLAN_PRIO,
					TCA_FLOWER_KEY_CVLAN_ETH_TYPE,
					&key->cvlan, &mask->cvlan);
			fl_set_key_val(tb, &key->basic.n_proto,
				       TCA_FLOWER_KEY_CVLAN_ETH_TYPE,
				       &mask->basic.n_proto,
				       TCA_FLOWER_UNSPEC,
				       sizeof(key->basic.n_proto));
		}
	}

	if (key->basic.n_proto == htons(ETH_P_PPP_SES))
		fl_set_key_pppoe(tb, &key->pppoe, &mask->pppoe, key, mask);

	if (key->basic.n_proto == htons(ETH_P_IP) ||
	    key->basic.n_proto == htons(ETH_P_IPV6)) {
		fl_set_key_val(tb, &key->basic.ip_proto, TCA_FLOWER_KEY_IP_PROTO,
			       &mask->basic.ip_proto, TCA_FLOWER_UNSPEC,
			       sizeof(key->basic.ip_proto));
		fl_set_key_ip(tb, false, &key->ip, &mask->ip);
	}

	if (tb[TCA_FLOWER_KEY_IPV4_SRC] || tb[TCA_FLOWER_KEY_IPV4_DST]) {
		key->control.addr_type = FLOW_DISSECTOR_KEY_IPV4_ADDRS;
		mask->control.addr_type = ~0;
		fl_set_key_val(tb, &key->ipv4.src, TCA_FLOWER_KEY_IPV4_SRC,
			       &mask->ipv4.src, TCA_FLOWER_KEY_IPV4_SRC_MASK,
			       sizeof(key->ipv4.src));
		fl_set_key_val(tb, &key->ipv4.dst, TCA_FLOWER_KEY_IPV4_DST,
			       &mask->ipv4.dst, TCA_FLOWER_KEY_IPV4_DST_MASK,
			       sizeof(key->ipv4.dst));
	} else if (tb[TCA_FLOWER_KEY_IPV6_SRC] || tb[TCA_FLOWER_KEY_IPV6_DST]) {
		key->control.addr_type = FLOW_DISSECTOR_KEY_IPV6_ADDRS;
		mask->control.addr_type = ~0;
		fl_set_key_val(tb, &key->ipv6.src, TCA_FLOWER_KEY_IPV6_SRC,
			       &mask->ipv6.src, TCA_FLOWER_KEY_IPV6_SRC_MASK,
			       sizeof(key->ipv6.src));
		fl_set_key_val(tb, &key->ipv6.dst, TCA_FLOWER_KEY_IPV6_DST,
			       &mask->ipv6.dst, TCA_FLOWER_KEY_IPV6_DST_MASK,
			       sizeof(key->ipv6.dst));
	}

	if (key->basic.ip_proto == IPPROTO_TCP) {
		fl_set_key_val(tb, &key->tp.src, TCA_FLOWER_KEY_TCP_SRC,
			       &mask->tp.src, TCA_FLOWER_KEY_TCP_SRC_MASK,
			       sizeof(key->tp.src));
		fl_set_key_val(tb, &key->tp.dst, TCA_FLOWER_KEY_TCP_DST,
			       &mask->tp.dst, TCA_FLOWER_KEY_TCP_DST_MASK,
			       sizeof(key->tp.dst));
		fl_set_key_val(tb, &key->tcp.flags, TCA_FLOWER_KEY_TCP_FLAGS,
			       &mask->tcp.flags, TCA_FLOWER_KEY_TCP_FLAGS_MASK,
			       sizeof(key->tcp.flags));
	} else if (key->basic.ip_proto == IPPROTO_UDP) {
		fl_set_key_val(tb, &key->tp.src, TCA_FLOWER_KEY_UDP_SRC,
			       &mask->tp.src, TCA_FLOWER_KEY_UDP_SRC_MASK,
			       sizeof(key->tp.src));
		fl_set_key_val(tb, &key->tp.dst, TCA_FLOWER_KEY_UDP_DST,
			       &mask->tp.dst, TCA_FLOWER_KEY_UDP_DST_MASK,
			       sizeof(key->tp.dst));
	} else if (key->basic.ip_proto == IPPROTO_SCTP) {
		fl_set_key_val(tb, &key->tp.src, TCA_FLOWER_KEY_SCTP_SRC,
			       &mask->tp.src, TCA_FLOWER_KEY_SCTP_SRC_MASK,
			       sizeof(key->tp.src));
		fl_set_key_val(tb, &key->tp.dst, TCA_FLOWER_KEY_SCTP_DST,
			       &mask->tp.dst, TCA_FLOWER_KEY_SCTP_DST_MASK,
			       sizeof(key->tp.dst));
	} else if (key->basic.n_proto == htons(ETH_P_IP) &&
		   key->basic.ip_proto == IPPROTO_ICMP) {
		fl_set_key_val(tb, &key->icmp.type, TCA_FLOWER_KEY_ICMPV4_TYPE,
			       &mask->icmp.type,
			       TCA_FLOWER_KEY_ICMPV4_TYPE_MASK,
			       sizeof(key->icmp.type));
		fl_set_key_val(tb, &key->icmp.code, TCA_FLOWER_KEY_ICMPV4_CODE,
			       &mask->icmp.code,
			       TCA_FLOWER_KEY_ICMPV4_CODE_MASK,
			       sizeof(key->icmp.code));
	} else if (key->basic.n_proto == htons(ETH_P_IPV6) &&
		   key->basic.ip_proto == IPPROTO_ICMPV6) {
		fl_set_key_val(tb, &key->icmp.type, TCA_FLOWER_KEY_ICMPV6_TYPE,
			       &mask->icmp.type,
			       TCA_FLOWER_KEY_ICMPV6_TYPE_MASK,
			       sizeof(key->icmp.type));
		fl_set_key_val(tb, &key->icmp.code, TCA_FLOWER_KEY_ICMPV6_CODE,
			       &mask->icmp.code,
			       TCA_FLOWER_KEY_ICMPV6_CODE_MASK,
			       sizeof(key->icmp.code));
	} else if (key->basic.n_proto == htons(ETH_P_MPLS_UC) ||
		   key->basic.n_proto == htons(ETH_P_MPLS_MC)) {
		ret = fl_set_key_mpls(tb, &key->mpls, &mask->mpls, extack);
		if (ret)
			return ret;
	} else if (key->basic.n_proto == htons(ETH_P_ARP) ||
		   key->basic.n_proto == htons(ETH_P_RARP)) {
		fl_set_key_val(tb, &key->arp.sip, TCA_FLOWER_KEY_ARP_SIP,
			       &mask->arp.sip, TCA_FLOWER_KEY_ARP_SIP_MASK,
			       sizeof(key->arp.sip));
		fl_set_key_val(tb, &key->arp.tip, TCA_FLOWER_KEY_ARP_TIP,
			       &mask->arp.tip, TCA_FLOWER_KEY_ARP_TIP_MASK,
			       sizeof(key->arp.tip));
		fl_set_key_val(tb, &key->arp.op, TCA_FLOWER_KEY_ARP_OP,
			       &mask->arp.op, TCA_FLOWER_KEY_ARP_OP_MASK,
			       sizeof(key->arp.op));
		fl_set_key_val(tb, key->arp.sha, TCA_FLOWER_KEY_ARP_SHA,
			       mask->arp.sha, TCA_FLOWER_KEY_ARP_SHA_MASK,
			       sizeof(key->arp.sha));
		fl_set_key_val(tb, key->arp.tha, TCA_FLOWER_KEY_ARP_THA,
			       mask->arp.tha, TCA_FLOWER_KEY_ARP_THA_MASK,
			       sizeof(key->arp.tha));
	} else if (key->basic.ip_proto == IPPROTO_L2TP) {
		fl_set_key_val(tb, &key->l2tpv3.session_id,
			       TCA_FLOWER_KEY_L2TPV3_SID,
			       &mask->l2tpv3.session_id, TCA_FLOWER_UNSPEC,
			       sizeof(key->l2tpv3.session_id));
	} else if (key->basic.n_proto  == htons(ETH_P_CFM)) {
		ret = fl_set_key_cfm(tb, key, mask, extack);
		if (ret)
			return ret;
	}

	if (key->basic.ip_proto == IPPROTO_TCP ||
	    key->basic.ip_proto == IPPROTO_UDP ||
	    key->basic.ip_proto == IPPROTO_SCTP) {
		ret = fl_set_key_port_range(tb, key, mask, extack);
		if (ret)
			return ret;
	}

	if (tb[TCA_FLOWER_KEY_SPI]) {
		ret = fl_set_key_spi(tb, key, mask, extack);
		if (ret)
			return ret;
	}

	if (tb[TCA_FLOWER_KEY_ENC_IPV4_SRC] ||
	    tb[TCA_FLOWER_KEY_ENC_IPV4_DST]) {
		key->enc_control.addr_type = FLOW_DISSECTOR_KEY_IPV4_ADDRS;
		mask->enc_control.addr_type = ~0;
		fl_set_key_val(tb, &key->enc_ipv4.src,
			       TCA_FLOWER_KEY_ENC_IPV4_SRC,
			       &mask->enc_ipv4.src,
			       TCA_FLOWER_KEY_ENC_IPV4_SRC_MASK,
			       sizeof(key->enc_ipv4.src));
		fl_set_key_val(tb, &key->enc_ipv4.dst,
			       TCA_FLOWER_KEY_ENC_IPV4_DST,
			       &mask->enc_ipv4.dst,
			       TCA_FLOWER_KEY_ENC_IPV4_DST_MASK,
			       sizeof(key->enc_ipv4.dst));
	}

	if (tb[TCA_FLOWER_KEY_ENC_IPV6_SRC] ||
	    tb[TCA_FLOWER_KEY_ENC_IPV6_DST]) {
		key->enc_control.addr_type = FLOW_DISSECTOR_KEY_IPV6_ADDRS;
		mask->enc_control.addr_type = ~0;
		fl_set_key_val(tb, &key->enc_ipv6.src,
			       TCA_FLOWER_KEY_ENC_IPV6_SRC,
			       &mask->enc_ipv6.src,
			       TCA_FLOWER_KEY_ENC_IPV6_SRC_MASK,
			       sizeof(key->enc_ipv6.src));
		fl_set_key_val(tb, &key->enc_ipv6.dst,
			       TCA_FLOWER_KEY_ENC_IPV6_DST,
			       &mask->enc_ipv6.dst,
			       TCA_FLOWER_KEY_ENC_IPV6_DST_MASK,
			       sizeof(key->enc_ipv6.dst));
	}

	fl_set_key_val(tb, &key->enc_key_id.keyid, TCA_FLOWER_KEY_ENC_KEY_ID,
		       &mask->enc_key_id.keyid, TCA_FLOWER_UNSPEC,
		       sizeof(key->enc_key_id.keyid));

	fl_set_key_val(tb, &key->enc_tp.src, TCA_FLOWER_KEY_ENC_UDP_SRC_PORT,
		       &mask->enc_tp.src, TCA_FLOWER_KEY_ENC_UDP_SRC_PORT_MASK,
		       sizeof(key->enc_tp.src));

	fl_set_key_val(tb, &key->enc_tp.dst, TCA_FLOWER_KEY_ENC_UDP_DST_PORT,
		       &mask->enc_tp.dst, TCA_FLOWER_KEY_ENC_UDP_DST_PORT_MASK,
		       sizeof(key->enc_tp.dst));

	fl_set_key_ip(tb, true, &key->enc_ip, &mask->enc_ip);

	fl_set_key_val(tb, &key->hash.hash, TCA_FLOWER_KEY_HASH,
		       &mask->hash.hash, TCA_FLOWER_KEY_HASH_MASK,
		       sizeof(key->hash.hash));

	if (tb[TCA_FLOWER_KEY_ENC_OPTS]) {
		ret = fl_set_enc_opt(tb, key, mask, extack);
		if (ret)
			return ret;
	}

	ret = fl_set_key_ct(tb, &key->ct, &mask->ct, extack);
	if (ret)
		return ret;

	if (tb[TCA_FLOWER_KEY_FLAGS]) {
		ret = fl_set_key_flags(tca_opts, tb, false,
				       &key->control.flags,
				       &mask->control.flags, extack);
		if (ret)
			return ret;
	}

	if (tb[TCA_FLOWER_KEY_ENC_FLAGS])
		ret = fl_set_key_flags(tca_opts, tb, true,
				       &key->enc_control.flags,
				       &mask->enc_control.flags, extack);

	return ret;
}
/* Context-After
 * static void fl_mask_copy(struct fl_flow_mask *dst,
 * 			 struct fl_flow_mask *src)
 * {
 * 	const void *psrc = fl_key_get_start(&src->key, src);
 * 	void *pdst = fl_key_get_start(&dst->key, src);
 * 
 * 	memcpy(pdst, psrc, fl_mask_range(src));
 * 	dst->range = src->range;
 * }
 * 
 * static const struct rhashtable_params fl_ht_params = {
 * 	.key_offset = offsetof(struct cls_fl_filter, mkey), /* base offset */
 * 	.head_offset = offsetof(struct cls_fl_filter, ht_node),
 * 	.automatic_shrinking = true,
 * };
 * 
 * static int fl_init_mask_hashtable(struct fl_flow_mask *mask)
 * {
 * 	mask->filter_ht_params = fl_ht_params;
 */
