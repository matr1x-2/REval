/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_044 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 35 */
/* NLOC: 203 */
/* Subsystem: net */
/* Includes
 * #include "flow.h"
 * #include "datapath.h"
 * #include <linux/uaccess.h>
 * #include <linux/netdevice.h>
 * #include <linux/etherdevice.h>
 * #include <linux/if_ether.h>
 * #include <linux/if_vlan.h>
 * #include <net/llc_pdu.h>
 * #include <linux/kernel.h>
 * #include <linux/jhash.h>
 * #include <linux/jiffies.h>
 * #include <linux/llc.h>
 * #include <linux/module.h>
 * #include <linux/in.h>
 * #include <linux/rcupdate.h>
 * #include <linux/if_arp.h>
 * #include <linux/ip.h>
 * #include <linux/ipv6.h>
 * #include <linux/sctp.h>
 * #include <linux/tcp.h>
 */
/* Context-Before
 * 		if ((has_md1 && mdtype != NSH_M_TYPE1) ||
 * 		    (has_md2 && mdtype != NSH_M_TYPE2)) {
 * 			OVS_NLERR(1, "nsh attribute has unmatched MD type %d.",
 * 				  mdtype);
 * 			return -EINVAL;
 * 		}
 * 
 * 		if (is_push_nsh &&
 * 		    (!has_base || (!has_md1 && !has_md2))) {
 * 			OVS_NLERR(
 * 			    1,
 * 			    "push_nsh: missing base or metadata attributes"
 * 			);
 * 			return -EINVAL;
 * 		}
 * 	}
 * 
 * 	return 0;
 * }
 */
static int ovs_key_from_nlattrs(struct net *net, struct sw_flow_match *match,
				u64 attrs, const struct nlattr **a,
				bool is_mask, bool log)
{
	int err;

	err = metadata_from_nlattrs(net, match, &attrs, a, is_mask, log);
	if (err)
		return err;

	if (attrs & (1 << OVS_KEY_ATTR_ETHERNET)) {
		const struct ovs_key_ethernet *eth_key;

		eth_key = nla_data(a[OVS_KEY_ATTR_ETHERNET]);
		SW_FLOW_KEY_MEMCPY(match, eth.src,
				eth_key->eth_src, ETH_ALEN, is_mask);
		SW_FLOW_KEY_MEMCPY(match, eth.dst,
				eth_key->eth_dst, ETH_ALEN, is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_ETHERNET);

		if (attrs & (1 << OVS_KEY_ATTR_VLAN)) {
			/* VLAN attribute is always parsed before getting here since it
			 * may occur multiple times.
			 */
			OVS_NLERR(log, "VLAN attribute unexpected.");
			return -EINVAL;
		}

		if (attrs & (1 << OVS_KEY_ATTR_ETHERTYPE)) {
			err = parse_eth_type_from_nlattrs(match, &attrs, a, is_mask,
							  log);
			if (err)
				return err;
		} else if (!is_mask) {
			SW_FLOW_KEY_PUT(match, eth.type, htons(ETH_P_802_2), is_mask);
		}
	} else if (!match->key->eth.type) {
		OVS_NLERR(log, "Either Ethernet header or EtherType is required.");
		return -EINVAL;
	}

	if (attrs & (1 << OVS_KEY_ATTR_IPV4)) {
		const struct ovs_key_ipv4 *ipv4_key;

		ipv4_key = nla_data(a[OVS_KEY_ATTR_IPV4]);
		if (!is_mask && ipv4_key->ipv4_frag > OVS_FRAG_TYPE_MAX) {
			OVS_NLERR(log, "IPv4 frag type %d is out of range max %d",
				  ipv4_key->ipv4_frag, OVS_FRAG_TYPE_MAX);
			return -EINVAL;
		}
		SW_FLOW_KEY_PUT(match, ip.proto,
				ipv4_key->ipv4_proto, is_mask);
		SW_FLOW_KEY_PUT(match, ip.tos,
				ipv4_key->ipv4_tos, is_mask);
		SW_FLOW_KEY_PUT(match, ip.ttl,
				ipv4_key->ipv4_ttl, is_mask);
		SW_FLOW_KEY_PUT(match, ip.frag,
				ipv4_key->ipv4_frag, is_mask);
		SW_FLOW_KEY_PUT(match, ipv4.addr.src,
				ipv4_key->ipv4_src, is_mask);
		SW_FLOW_KEY_PUT(match, ipv4.addr.dst,
				ipv4_key->ipv4_dst, is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_IPV4);
	}

	if (attrs & (1 << OVS_KEY_ATTR_IPV6)) {
		const struct ovs_key_ipv6 *ipv6_key;

		ipv6_key = nla_data(a[OVS_KEY_ATTR_IPV6]);
		if (!is_mask && ipv6_key->ipv6_frag > OVS_FRAG_TYPE_MAX) {
			OVS_NLERR(log, "IPv6 frag type %d is out of range max %d",
				  ipv6_key->ipv6_frag, OVS_FRAG_TYPE_MAX);
			return -EINVAL;
		}

		if (!is_mask && ipv6_key->ipv6_label & htonl(0xFFF00000)) {
			OVS_NLERR(log, "IPv6 flow label %x is out of range (max=%x)",
				  ntohl(ipv6_key->ipv6_label), (1 << 20) - 1);
			return -EINVAL;
		}

		SW_FLOW_KEY_PUT(match, ipv6.label,
				ipv6_key->ipv6_label, is_mask);
		SW_FLOW_KEY_PUT(match, ip.proto,
				ipv6_key->ipv6_proto, is_mask);
		SW_FLOW_KEY_PUT(match, ip.tos,
				ipv6_key->ipv6_tclass, is_mask);
		SW_FLOW_KEY_PUT(match, ip.ttl,
				ipv6_key->ipv6_hlimit, is_mask);
		SW_FLOW_KEY_PUT(match, ip.frag,
				ipv6_key->ipv6_frag, is_mask);
		SW_FLOW_KEY_MEMCPY(match, ipv6.addr.src,
				ipv6_key->ipv6_src,
				sizeof(match->key->ipv6.addr.src),
				is_mask);
		SW_FLOW_KEY_MEMCPY(match, ipv6.addr.dst,
				ipv6_key->ipv6_dst,
				sizeof(match->key->ipv6.addr.dst),
				is_mask);

		attrs &= ~(1 << OVS_KEY_ATTR_IPV6);
	}

	if (attrs & (1ULL << OVS_KEY_ATTR_IPV6_EXTHDRS)) {
		const struct ovs_key_ipv6_exthdrs *ipv6_exthdrs_key;

		ipv6_exthdrs_key = nla_data(a[OVS_KEY_ATTR_IPV6_EXTHDRS]);

		SW_FLOW_KEY_PUT(match, ipv6.exthdrs,
				ipv6_exthdrs_key->hdrs, is_mask);

		attrs &= ~(1ULL << OVS_KEY_ATTR_IPV6_EXTHDRS);
	}

	if (attrs & (1 << OVS_KEY_ATTR_ARP)) {
		const struct ovs_key_arp *arp_key;

		arp_key = nla_data(a[OVS_KEY_ATTR_ARP]);
		if (!is_mask && (arp_key->arp_op & htons(0xff00))) {
			OVS_NLERR(log, "Unknown ARP opcode (opcode=%d).",
				  arp_key->arp_op);
			return -EINVAL;
		}

		SW_FLOW_KEY_PUT(match, ipv4.addr.src,
				arp_key->arp_sip, is_mask);
		SW_FLOW_KEY_PUT(match, ipv4.addr.dst,
			arp_key->arp_tip, is_mask);
		SW_FLOW_KEY_PUT(match, ip.proto,
				ntohs(arp_key->arp_op), is_mask);
		SW_FLOW_KEY_MEMCPY(match, ipv4.arp.sha,
				arp_key->arp_sha, ETH_ALEN, is_mask);
		SW_FLOW_KEY_MEMCPY(match, ipv4.arp.tha,
				arp_key->arp_tha, ETH_ALEN, is_mask);

		attrs &= ~(1 << OVS_KEY_ATTR_ARP);
	}

	if (attrs & (1 << OVS_KEY_ATTR_NSH)) {
		if (nsh_key_put_from_nlattr(a[OVS_KEY_ATTR_NSH], match,
					    is_mask, false, log) < 0)
			return -EINVAL;
		attrs &= ~(1 << OVS_KEY_ATTR_NSH);
	}

	if (attrs & (1 << OVS_KEY_ATTR_MPLS)) {
		const struct ovs_key_mpls *mpls_key;
		u32 hdr_len;
		u32 label_count, label_count_mask, i;

		mpls_key = nla_data(a[OVS_KEY_ATTR_MPLS]);
		hdr_len = nla_len(a[OVS_KEY_ATTR_MPLS]);
		label_count = hdr_len / sizeof(struct ovs_key_mpls);

		if (label_count == 0 || label_count > MPLS_LABEL_DEPTH ||
		    hdr_len % sizeof(struct ovs_key_mpls))
			return -EINVAL;

		label_count_mask =  GENMASK(label_count - 1, 0);

		for (i = 0 ; i < label_count; i++)
			SW_FLOW_KEY_PUT(match, mpls.lse[i],
					mpls_key[i].mpls_lse, is_mask);

		SW_FLOW_KEY_PUT(match, mpls.num_labels_mask,
				label_count_mask, is_mask);

		attrs &= ~(1 << OVS_KEY_ATTR_MPLS);
	 }

	if (attrs & (1 << OVS_KEY_ATTR_TCP)) {
		const struct ovs_key_tcp *tcp_key;

		tcp_key = nla_data(a[OVS_KEY_ATTR_TCP]);
		SW_FLOW_KEY_PUT(match, tp.src, tcp_key->tcp_src, is_mask);
		SW_FLOW_KEY_PUT(match, tp.dst, tcp_key->tcp_dst, is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_TCP);
	}

	if (attrs & (1 << OVS_KEY_ATTR_TCP_FLAGS)) {
		SW_FLOW_KEY_PUT(match, tp.flags,
				nla_get_be16(a[OVS_KEY_ATTR_TCP_FLAGS]),
				is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_TCP_FLAGS);
	}

	if (attrs & (1 << OVS_KEY_ATTR_UDP)) {
		const struct ovs_key_udp *udp_key;

		udp_key = nla_data(a[OVS_KEY_ATTR_UDP]);
		SW_FLOW_KEY_PUT(match, tp.src, udp_key->udp_src, is_mask);
		SW_FLOW_KEY_PUT(match, tp.dst, udp_key->udp_dst, is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_UDP);
	}

	if (attrs & (1 << OVS_KEY_ATTR_SCTP)) {
		const struct ovs_key_sctp *sctp_key;

		sctp_key = nla_data(a[OVS_KEY_ATTR_SCTP]);
		SW_FLOW_KEY_PUT(match, tp.src, sctp_key->sctp_src, is_mask);
		SW_FLOW_KEY_PUT(match, tp.dst, sctp_key->sctp_dst, is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_SCTP);
	}

	if (attrs & (1 << OVS_KEY_ATTR_ICMP)) {
		const struct ovs_key_icmp *icmp_key;

		icmp_key = nla_data(a[OVS_KEY_ATTR_ICMP]);
		SW_FLOW_KEY_PUT(match, tp.src,
				htons(icmp_key->icmp_type), is_mask);
		SW_FLOW_KEY_PUT(match, tp.dst,
				htons(icmp_key->icmp_code), is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_ICMP);
	}

	if (attrs & (1 << OVS_KEY_ATTR_ICMPV6)) {
		const struct ovs_key_icmpv6 *icmpv6_key;

		icmpv6_key = nla_data(a[OVS_KEY_ATTR_ICMPV6]);
		SW_FLOW_KEY_PUT(match, tp.src,
				htons(icmpv6_key->icmpv6_type), is_mask);
		SW_FLOW_KEY_PUT(match, tp.dst,
				htons(icmpv6_key->icmpv6_code), is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_ICMPV6);
	}

	if (attrs & (1 << OVS_KEY_ATTR_ND)) {
		const struct ovs_key_nd *nd_key;

		nd_key = nla_data(a[OVS_KEY_ATTR_ND]);
		SW_FLOW_KEY_MEMCPY(match, ipv6.nd.target,
			nd_key->nd_target,
			sizeof(match->key->ipv6.nd.target),
			is_mask);
		SW_FLOW_KEY_MEMCPY(match, ipv6.nd.sll,
			nd_key->nd_sll, ETH_ALEN, is_mask);
		SW_FLOW_KEY_MEMCPY(match, ipv6.nd.tll,
				nd_key->nd_tll, ETH_ALEN, is_mask);
		attrs &= ~(1 << OVS_KEY_ATTR_ND);
	}

	if (attrs != 0) {
		OVS_NLERR(log, "Unknown key attributes %llx",
			  (unsigned long long)attrs);
		return -EINVAL;
	}

	return 0;
}
/* Context-After
 * static void nlattr_set(struct nlattr *attr, u8 val,
 * 		       const struct ovs_len_tbl *tbl)
 * {
 * 	struct nlattr *nla;
 * 	int rem;
 * 
 * 	/* The nlattr stream should already have been validated */
 * 	nla_for_each_nested(nla, attr, rem) {
 * 		if (tbl[nla_type(nla)].len == OVS_ATTR_NESTED)
 * 			nlattr_set(nla, val, tbl[nla_type(nla)].next ? : tbl);
 * 		else
 * 			memset(nla_data(nla), val, nla_len(nla));
 * 
 * 		if (nla_type(nla) == OVS_KEY_ATTR_CT_STATE)
 * 			*(u32 *)nla_data(nla) &= CT_SUPPORTED_MASK;
 * 	}
 * }
 * 
 * static void mask_set_nlattr(struct nlattr *attr, u8 val)
 */
