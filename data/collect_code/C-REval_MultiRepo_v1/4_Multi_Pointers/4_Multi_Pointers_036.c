/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_036 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 30 */
/* NLOC: 97 */
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
 * 	[OVS_CT_ATTR_COMMIT]	= { .minlen = 0, .maxlen = 0 },
 * 	[OVS_CT_ATTR_FORCE_COMMIT]	= { .minlen = 0, .maxlen = 0 },
 * 	[OVS_CT_ATTR_ZONE]	= { .minlen = sizeof(u16),
 * 				    .maxlen = sizeof(u16) },
 * 	[OVS_CT_ATTR_MARK]	= { .minlen = sizeof(struct md_mark),
 * 				    .maxlen = sizeof(struct md_mark) },
 * 	[OVS_CT_ATTR_LABELS]	= { .minlen = sizeof(struct md_labels),
 * 				    .maxlen = sizeof(struct md_labels) },
 * 	[OVS_CT_ATTR_HELPER]	= { .minlen = 1,
 * 				    .maxlen = NF_CT_HELPER_NAME_LEN },
 * #if IS_ENABLED(CONFIG_NF_NAT)
 * 	/* NAT length is checked when parsing the nested attributes. */
 * 	[OVS_CT_ATTR_NAT]	= { .minlen = 0, .maxlen = INT_MAX },
 * #endif
 * 	[OVS_CT_ATTR_EVENTMASK]	= { .minlen = sizeof(u32),
 * 				    .maxlen = sizeof(u32) },
 * 	[OVS_CT_ATTR_TIMEOUT] = { .minlen = 1,
 * 				  .maxlen = CTNL_TIMEOUT_NAME_MAX },
 * };
 */
static int parse_ct(const struct nlattr *attr, struct ovs_conntrack_info *info,
		    const char **helper, bool log)
{
	struct nlattr *a;
	int rem;

	nla_for_each_nested(a, attr, rem) {
		int type = nla_type(a);
		int maxlen;
		int minlen;

		if (type > OVS_CT_ATTR_MAX) {
			OVS_NLERR(log,
				  "Unknown conntrack attr (type=%d, max=%d)",
				  type, OVS_CT_ATTR_MAX);
			return -EINVAL;
		}

		maxlen = ovs_ct_attr_lens[type].maxlen;
		minlen = ovs_ct_attr_lens[type].minlen;
		if (nla_len(a) < minlen || nla_len(a) > maxlen) {
			OVS_NLERR(log,
				  "Conntrack attr type has unexpected length (type=%d, length=%d, expected=%d)",
				  type, nla_len(a), maxlen);
			return -EINVAL;
		}

		switch (type) {
		case OVS_CT_ATTR_FORCE_COMMIT:
			info->force = true;
			fallthrough;
		case OVS_CT_ATTR_COMMIT:
			info->commit = true;
			break;
#ifdef CONFIG_NF_CONNTRACK_ZONES
		case OVS_CT_ATTR_ZONE:
			info->zone.id = nla_get_u16(a);
			break;
#endif
#ifdef CONFIG_NF_CONNTRACK_MARK
		case OVS_CT_ATTR_MARK: {
			struct md_mark *mark = nla_data(a);

			if (!mark->mask) {
				OVS_NLERR(log, "ct_mark mask cannot be 0");
				return -EINVAL;
			}
			info->mark = *mark;
			break;
		}
#endif
#ifdef CONFIG_NF_CONNTRACK_LABELS
		case OVS_CT_ATTR_LABELS: {
			struct md_labels *labels = nla_data(a);

			if (!labels_nonzero(&labels->mask)) {
				OVS_NLERR(log, "ct_labels mask cannot be 0");
				return -EINVAL;
			}
			info->labels = *labels;
			break;
		}
#endif
		case OVS_CT_ATTR_HELPER:
			*helper = nla_data(a);
			if (!string_is_terminated(*helper, nla_len(a))) {
				OVS_NLERR(log, "Invalid conntrack helper");
				return -EINVAL;
			}
			break;
#if IS_ENABLED(CONFIG_NF_NAT)
		case OVS_CT_ATTR_NAT: {
			int err = parse_nat(a, info, log);

			if (err)
				return err;
			break;
		}
#endif
		case OVS_CT_ATTR_EVENTMASK:
			info->have_eventmask = true;
			info->eventmask = nla_get_u32(a);
			break;
#ifdef CONFIG_NF_CONNTRACK_TIMEOUT
		case OVS_CT_ATTR_TIMEOUT:
			memcpy(info->timeout, nla_data(a), nla_len(a));
			if (!string_is_terminated(info->timeout, nla_len(a))) {
				OVS_NLERR(log, "Invalid conntrack timeout");
				return -EINVAL;
			}
			break;
#endif

		default:
			OVS_NLERR(log, "Unknown conntrack attr (%d)",
				  type);
			return -EINVAL;
		}
	}

#ifdef CONFIG_NF_CONNTRACK_MARK
	if (!info->commit && info->mark.mask) {
		OVS_NLERR(log,
			  "Setting conntrack mark requires 'commit' flag.");
		return -EINVAL;
	}
#endif
#ifdef CONFIG_NF_CONNTRACK_LABELS
	if (!info->commit && labels_nonzero(&info->labels.mask)) {
		OVS_NLERR(log,
			  "Setting conntrack labels requires 'commit' flag.");
		return -EINVAL;
	}
#endif
	if (rem > 0) {
		OVS_NLERR(log, "Conntrack attr has %d unknown bytes", rem);
		return -EINVAL;
	}

	return 0;
}
/* Context-After
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
