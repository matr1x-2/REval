/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_028 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 25 */
/* NLOC: 108 */
/* Subsystem: net */
/* Includes
 * #include <linux/types.h>
 * #include <linux/socket.h>
 * #include <linux/string.h>
 * #include <linux/skbuff.h>
 * #include <linux/in.h>
 * #include <linux/in6.h>
 * #include <linux/slab.h>
 * #include <net/sock.h>
 * #include <net/netlink.h>
 * #include <net/genetlink.h>
 * #include <net/ip.h>
 * #include <net/ipv6.h>
 * #include <net/netlabel.h>
 * #include <net/cipso_ipv4.h>
 * #include <net/calipso.h>
 * #include <linux/atomic.h>
 * #include "netlabel_calipso.h"
 * #include "netlabel_domainhash.h"
 * #include "netlabel_user.h"
 * #include "netlabel_mgmt.h"
 */
/* Context-Before
 * 	calipso_doi_putdef(calipso);
 * #endif
 * add_free_domain:
 * 	kfree(entry->domain);
 * add_free_entry:
 * 	kfree(entry);
 * 	return ret_val;
 * }
 * 
 * /**
 *  * netlbl_mgmt_listentry - List a NetLabel/LSM domain map entry
 *  * @skb: the NETLINK buffer
 *  * @entry: the map entry
 *  *
 *  * Description:
 *  * This function is a helper function used by the LISTALL and LISTDEF command
 *  * handlers.  The caller is responsible for ensuring that the RCU read lock
 *  * is held.  Returns zero on success, negative values on failure.
 *  *
 *  */
 */
static int netlbl_mgmt_listentry(struct sk_buff *skb,
				 struct netlbl_dom_map *entry)
{
	int ret_val = 0;
	struct nlattr *nla_a;
	struct nlattr *nla_b;
	struct netlbl_af4list *iter4;
#if IS_ENABLED(CONFIG_IPV6)
	struct netlbl_af6list *iter6;
#endif

	if (entry->domain != NULL) {
		ret_val = nla_put_string(skb,
					 NLBL_MGMT_A_DOMAIN, entry->domain);
		if (ret_val != 0)
			return ret_val;
	}

	ret_val = nla_put_u16(skb, NLBL_MGMT_A_FAMILY, entry->family);
	if (ret_val != 0)
		return ret_val;

	switch (entry->def.type) {
	case NETLBL_NLTYPE_ADDRSELECT:
		nla_a = nla_nest_start_noflag(skb, NLBL_MGMT_A_SELECTORLIST);
		if (nla_a == NULL)
			return -ENOMEM;

		netlbl_af4list_foreach_rcu(iter4, &entry->def.addrsel->list4) {
			struct netlbl_domaddr4_map *map4;
			struct in_addr addr_struct;

			nla_b = nla_nest_start_noflag(skb,
						      NLBL_MGMT_A_ADDRSELECTOR);
			if (nla_b == NULL)
				return -ENOMEM;

			addr_struct.s_addr = iter4->addr;
			ret_val = nla_put_in_addr(skb, NLBL_MGMT_A_IPV4ADDR,
						  addr_struct.s_addr);
			if (ret_val != 0)
				return ret_val;
			addr_struct.s_addr = iter4->mask;
			ret_val = nla_put_in_addr(skb, NLBL_MGMT_A_IPV4MASK,
						  addr_struct.s_addr);
			if (ret_val != 0)
				return ret_val;
			map4 = netlbl_domhsh_addr4_entry(iter4);
			ret_val = nla_put_u32(skb, NLBL_MGMT_A_PROTOCOL,
					      map4->def.type);
			if (ret_val != 0)
				return ret_val;
			switch (map4->def.type) {
			case NETLBL_NLTYPE_CIPSOV4:
				ret_val = nla_put_u32(skb, NLBL_MGMT_A_CV4DOI,
						      map4->def.cipso->doi);
				if (ret_val != 0)
					return ret_val;
				break;
			}

			nla_nest_end(skb, nla_b);
		}
#if IS_ENABLED(CONFIG_IPV6)
		netlbl_af6list_foreach_rcu(iter6, &entry->def.addrsel->list6) {
			struct netlbl_domaddr6_map *map6;

			nla_b = nla_nest_start_noflag(skb,
						      NLBL_MGMT_A_ADDRSELECTOR);
			if (nla_b == NULL)
				return -ENOMEM;

			ret_val = nla_put_in6_addr(skb, NLBL_MGMT_A_IPV6ADDR,
						   &iter6->addr);
			if (ret_val != 0)
				return ret_val;
			ret_val = nla_put_in6_addr(skb, NLBL_MGMT_A_IPV6MASK,
						   &iter6->mask);
			if (ret_val != 0)
				return ret_val;
			map6 = netlbl_domhsh_addr6_entry(iter6);
			ret_val = nla_put_u32(skb, NLBL_MGMT_A_PROTOCOL,
					      map6->def.type);
			if (ret_val != 0)
				return ret_val;

			switch (map6->def.type) {
			case NETLBL_NLTYPE_CALIPSO:
				ret_val = nla_put_u32(skb, NLBL_MGMT_A_CLPDOI,
						      map6->def.calipso->doi);
				if (ret_val != 0)
					return ret_val;
				break;
			}

			nla_nest_end(skb, nla_b);
		}
#endif /* IPv6 */

		nla_nest_end(skb, nla_a);
		break;
	case NETLBL_NLTYPE_UNLABELED:
		ret_val = nla_put_u32(skb, NLBL_MGMT_A_PROTOCOL,
				      entry->def.type);
		break;
	case NETLBL_NLTYPE_CIPSOV4:
		ret_val = nla_put_u32(skb, NLBL_MGMT_A_PROTOCOL,
				      entry->def.type);
		if (ret_val != 0)
			return ret_val;
		ret_val = nla_put_u32(skb, NLBL_MGMT_A_CV4DOI,
				      entry->def.cipso->doi);
		break;
	case NETLBL_NLTYPE_CALIPSO:
		ret_val = nla_put_u32(skb, NLBL_MGMT_A_PROTOCOL,
				      entry->def.type);
		if (ret_val != 0)
			return ret_val;
		ret_val = nla_put_u32(skb, NLBL_MGMT_A_CLPDOI,
				      entry->def.calipso->doi);
		break;
	}

	return ret_val;
}
/* Context-After
 * /*
 *  * NetLabel Command Handlers
 *  */
 * 
 * /**
 *  * netlbl_mgmt_add - Handle an ADD message
 *  * @skb: the NETLINK buffer
 *  * @info: the Generic NETLINK info block
 *  *
 *  * Description:
 *  * Process a user generated ADD message and add the domains from the message
 *  * to the hash table.  See netlabel.h for a description of the message format.
 *  * Returns zero on success, negative values on failure.
 *  *
 *  */
 * static int netlbl_mgmt_add(struct sk_buff *skb, struct genl_info *info)
 * {
 * 	struct netlbl_audit audit_info;
 */
