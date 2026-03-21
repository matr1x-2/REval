/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_002 */
/* Category: 1_High_Complexity */
/* Repo: lwip */
/* Cyclomatic Complexity: 56 */
/* NLOC: 103 */
/* Subsystem: src */
/* Includes
 * #include "lwip/opt.h"
 * #include "lwip/def.h"
 * #include "lwip/mem.h"
 * #include "lwip/netif.h"
 * #include "lwip/ip.h"
 * #include "lwip/ip6.h"
 * #include "lwip/ip6_addr.h"
 * #include "lwip/ip6_frag.h"
 * #include "lwip/icmp6.h"
 * #include "lwip/priv/raw_priv.h"
 * #include "lwip/udp.h"
 * #include "lwip/priv/tcp_priv.h"
 * #include "lwip/dhcp6.h"
 * #include "lwip/nd6.h"
 * #include "lwip/mld6.h"
 * #include "lwip/debug.h"
 * #include "lwip/stats.h"
 * #include LWIP_HOOK_FILENAME
 */
/* Context-Before
 * /**
 *  * Finds the appropriate network interface for a given IPv6 address. It tries to select
 *  * a netif following a sequence of heuristics:
 *  * 1) if there is only 1 netif, return it
 *  * 2) if the destination is a zoned address, match its zone to a netif
 *  * 3) if the either the source or destination address is a scoped address,
 *  *    match the source address's zone (if set) or address (if not) to a netif
 *  * 4) tries to match the destination subnet to a configured address
 *  * 5) tries to find a router-announced route
 *  * 6) tries to match the (unscoped) source address to the netif
 *  * 7) returns the default netif, if configured
 *  *
 *  * Note that each of the two given addresses may or may not be properly zoned.
 *  *
 *  * @param src the source IPv6 address, if known
 *  * @param dest the destination IPv6 address for which to find the route
 *  * @return the netif on which to send to reach dest
 *  */
 * struct netif *
 */
ip6_route(const ip6_addr_t *src, const ip6_addr_t *dest)
{
#if LWIP_SINGLE_NETIF
  LWIP_UNUSED_ARG(src);
  LWIP_UNUSED_ARG(dest);
#else /* LWIP_SINGLE_NETIF */
  struct netif *netif;
  s8_t i;

  LWIP_ASSERT_CORE_LOCKED();

  /* If single netif configuration, fast return. */
  if ((netif_list != NULL) && (netif_list->next == NULL)) {
    if (!netif_is_up(netif_list) || !netif_is_link_up(netif_list) ||
        (ip6_addr_has_zone(dest) && !ip6_addr_test_zone(dest, netif_list))) {
      return NULL;
    }
    return netif_list;
  }

#if LWIP_IPV6_SCOPES
  /* Special processing for zoned destination addresses. This includes link-
   * local unicast addresses and interface/link-local multicast addresses. Use
   * the zone to find a matching netif. If the address is not zoned, then there
   * is technically no "wrong" netif to choose, and we leave routing to other
   * rules; in most cases this should be the scoped-source rule below. */
  if (ip6_addr_has_zone(dest)) {
    IP6_ADDR_ZONECHECK(dest);
    /* Find a netif based on the zone. For custom mappings, one zone may map
     * to multiple netifs, so find one that can actually send a packet. */
    NETIF_FOREACH(netif) {
      if (ip6_addr_test_zone(dest, netif) &&
          netif_is_up(netif) && netif_is_link_up(netif)) {
        return netif;
      }
    }
    /* No matching netif found. Do no try to route to a different netif,
     * as that would be a zone violation, resulting in any packets sent to
     * that netif being dropped on output. */
    return NULL;
  }
#endif /* LWIP_IPV6_SCOPES */

  /* Special processing for scoped source and destination addresses. If we get
   * here, the destination address does not have a zone, so either way we need
   * to look at the source address, which may or may not have a zone. If it
   * does, the zone is restrictive: there is (typically) only one matching
   * netif for it, and we should avoid routing to any other netif as that would
   * result in guaranteed zone violations. For scoped source addresses that do
   * not have a zone, use (only) a netif that has that source address locally
   * assigned. This case also applies to the loopback source address, which has
   * an implied link-local scope. If only the destination address is scoped
   * (but, again, not zoned), we still want to use only the source address to
   * determine its zone because that's most likely what the user/application
   * wants, regardless of whether the source address is scoped. Finally, some
   * of this story also applies if scoping is disabled altogether. */
#if LWIP_IPV6_SCOPES
  if (ip6_addr_has_scope(dest, IP6_UNKNOWN) ||
      ip6_addr_has_scope(src, IP6_UNICAST) ||
#else /* LWIP_IPV6_SCOPES */
  if (ip6_addr_islinklocal(dest) || ip6_addr_ismulticast_iflocal(dest) ||
      ip6_addr_ismulticast_linklocal(dest) || ip6_addr_islinklocal(src) ||
#endif /* LWIP_IPV6_SCOPES */
      ip6_addr_isloopback(src)) {
#if LWIP_IPV6_SCOPES
    if (ip6_addr_has_zone(src)) {
      /* Find a netif matching the source zone (relatively cheap). */
      NETIF_FOREACH(netif) {
        if (netif_is_up(netif) && netif_is_link_up(netif) &&
            ip6_addr_test_zone(src, netif)) {
          return netif;
        }
      }
    } else
#endif /* LWIP_IPV6_SCOPES */
    {
      /* Find a netif matching the source address (relatively expensive). */
      NETIF_FOREACH(netif) {
        if (!netif_is_up(netif) || !netif_is_link_up(netif)) {
          continue;
        }
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
          if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
              ip6_addr_zoneless_eq(src, netif_ip6_addr(netif, i))) {
            return netif;
          }
        }
      }
    }
    /* Again, do not use any other netif in this case, as that could result in
     * zone boundary violations. */
    return NULL;
  }

  /* We come here only if neither source nor destination is scoped. */
  IP6_ADDR_ZONECHECK(src);

#ifdef LWIP_HOOK_IP6_ROUTE
  netif = LWIP_HOOK_IP6_ROUTE(src, dest);
  if (netif != NULL) {
    return netif;
  }
#endif

  /* See if the destination subnet matches a configured address. In accordance
   * with RFC 5942, dynamically configured addresses do not have an implied
   * local subnet, and thus should be considered /128 assignments. However, as
   * such, the destination address may still match a local address, and so we
   * still need to check for exact matches here. By (lwIP) policy, statically
   * configured addresses do always have an implied local /64 subnet. */
  NETIF_FOREACH(netif) {
    if (!netif_is_up(netif) || !netif_is_link_up(netif)) {
      continue;
    }
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
          ip6_addr_net_eq(dest, netif_ip6_addr(netif, i)) &&
          (netif_ip6_addr_isstatic(netif, i) ||
          ip6_addr_nethost_eq(dest, netif_ip6_addr(netif, i)))) {
        return netif;
      }
    }
  }

  /* Get the netif for a suitable router-announced route. */
  netif = nd6_find_route(dest);
  if (netif != NULL) {
    return netif;
  }

  /* Try with the netif that matches the source address. Given the earlier rule
   * for scoped source addresses, this applies to unscoped addresses only. */
  if (!ip6_addr_isany(src)) {
    NETIF_FOREACH(netif) {
      if (!netif_is_up(netif) || !netif_is_link_up(netif)) {
        continue;
      }
      for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
            ip6_addr_eq(src, netif_ip6_addr(netif, i))) {
          return netif;
        }
      }
    }
  }

#if LWIP_NETIF_LOOPBACK && !LWIP_HAVE_LOOPIF
  /* loopif is disabled, loopback traffic is passed through any netif */
  if (ip6_addr_isloopback(dest)) {
    /* don't check for link on loopback traffic */
    if (netif_default != NULL && netif_is_up(netif_default)) {
      return netif_default;
    }
    /* default netif is not up, just use any netif for loopback traffic */
    NETIF_FOREACH(netif) {
      if (netif_is_up(netif)) {
        return netif;
      }
    }
    return NULL;
  }
#endif /* LWIP_NETIF_LOOPBACK && !LWIP_HAVE_LOOPIF */
#endif /* !LWIP_SINGLE_NETIF */

  /* no matching netif found, use default netif, if up */
  if ((netif_default == NULL) || !netif_is_up(netif_default) || !netif_is_link_up(netif_default)) {
    return NULL;
  }
  return netif_default;
}
/* Context-After
 * /**
 *  * @ingroup ip6
 *  * Select the best IPv6 source address for a given destination IPv6 address.
 *  *
 *  * This implementation follows RFC 6724 Sec. 5 to the following extent:
 *  * - Rules 1, 2, 3: fully implemented
 *  * - Rules 4, 5, 5.5: not applicable
 *  * - Rule 6: not implemented
 *  * - Rule 7: not applicable
 *  * - Rule 8: limited to "prefer /64 subnet match over non-match"
 *  *
 *  * For Rule 2, we deliberately deviate from RFC 6724 Sec. 3.1 by considering
 *  * ULAs to be of smaller scope than global addresses, to avoid that a preferred
 *  * ULA is picked over a deprecated global address when given a global address
 *  * as destination, as that would likely result in broken two-way communication.
 *  *
 *  * As long as temporary addresses are not supported (as used in Rule 7), a
 *  * proper implementation of Rule 8 would obviate the need to implement Rule 6.
 *  *
 */
