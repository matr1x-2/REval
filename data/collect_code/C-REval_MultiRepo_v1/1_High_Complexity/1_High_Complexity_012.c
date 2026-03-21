/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_012 */
/* Category: 1_High_Complexity */
/* Repo: lwip */
/* Cyclomatic Complexity: 38 */
/* NLOC: 147 */
/* Subsystem: test */
/* Includes
 * #include "test_dhcp.h"
 * #include "lwip/netif.h"
 * #include "lwip/dhcp.h"
 * #include "lwip/prot/dhcp.h"
 * #include "lwip/etharp.h"
 * #include "lwip/inet.h"
 * #include "netif/ethernet.h"
 */
/* Context-Before
 *   fail_if((startpos + len) > p->tot_len);
 *   while (startpos > p->len && p->next) {
 *     startpos -= p->len;
 *     p = p->next;
 *   }
 *   fail_if(p == NULL);
 *   fail_unless(startpos + len <= p->len); /* All data we seek within same pbuf */
 * 
 *   found = 0;
 *   data = (u8_t*)p->payload;
 *   for (i = startpos; i <= (p->len - len); i++) {
 *     if (memcmp(&data[i], mem, len) == 0) {
 *       found = 1;
 *       break;
 *     }
 *   }
 *   fail_unless(found);
 * }
 */
static err_t lwip_tx_func(struct netif *netif, struct pbuf *p)
{
  fail_unless(netif == &net_test);
  txpacket++;

  if (debug) {
    struct pbuf *pp = p;
    /* Dump data */
    printf("TX data (pkt %d, len %d, tick %d)", txpacket, p->tot_len, tick);
    do {
      int i;
      for (i = 0; i < pp->len; i++) {
        printf(" %02X", ((u8_t *) pp->payload)[i]);
      }
      if (pp->next) {
        pp = pp->next;
      }
    } while (pp->next);
    printf("\n");
  }

  switch (tcase) {
  case TEST_LWIP_DHCP:
    switch (txpacket) {
    case 1:
    case 2:
      {
        const u8_t ipproto[] = { 0x08, 0x00 };
        const u8_t bootp_start[] = { 0x01, 0x01, 0x06, 0x00}; /* bootp request, eth, hwaddr len 6, 0 hops */
        const u8_t ipaddrs[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, ipproto, sizeof(ipproto)); /* eth level proto: ip */

        check_pkt(p, 42, bootp_start, sizeof(bootp_start));

        check_pkt(p, 53, ipaddrs, sizeof(ipaddrs));

        check_pkt(p, 70, netif->hwaddr, 6); /* mac addr inside bootp */

        check_pkt(p, 278, magic_cookie, sizeof(magic_cookie));

        /* Check dhcp message type, can be at different positions */
        if (txpacket == 1) {
          u8_t dhcp_discover_opt[] = { 0x35, 0x01, 0x01 };
          check_pkt_fuzzy(p, 282, dhcp_discover_opt, sizeof(dhcp_discover_opt));
        } else if (txpacket == 2) {
          u8_t dhcp_request_opt[] = { 0x35, 0x01, 0x03 };
          u8_t requested_ipaddr[] = { 0x32, 0x04, 0xc3, 0xaa, 0xbd, 0xc8 }; /* Ask for offered IP */

          check_pkt_fuzzy(p, 282, dhcp_request_opt, sizeof(dhcp_request_opt));
          check_pkt_fuzzy(p, 282, requested_ipaddr, sizeof(requested_ipaddr));
        }
        break;
      }
#if DHCP_TEST_NUM_ARP_FRAMES > 0
    case 3:
#if DHCP_TEST_NUM_ARP_FRAMES > 1
    case 4:
#if DHCP_TEST_NUM_ARP_FRAMES > 2
    case 5:
#if DHCP_TEST_NUM_ARP_FRAMES > 3
    case 6:
#if DHCP_TEST_NUM_ARP_FRAMES > 4
    case 7:
#endif
#endif
#endif
#endif
      {
        const u8_t arpproto[] = { 0x08, 0x06 };

        check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, arpproto, sizeof(arpproto)); /* eth level proto: ip */
        break;
      }
#endif
    default:
        fail();
        break;
    }
    break;

  case TEST_LWIP_DHCP_NAK:
    {
      const u8_t ipproto[] = { 0x08, 0x00 };
      const u8_t bootp_start[] = { 0x01, 0x01, 0x06, 0x00}; /* bootp request, eth, hwaddr len 6, 0 hops */
      const u8_t ipaddrs[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
      const u8_t dhcp_nak_opt[] = { 0x35, 0x01, 0x04 };
      const u8_t requested_ipaddr[] = { 0x32, 0x04, 0xc3, 0xaa, 0xbd, 0xc8 }; /* offered IP */

      fail_unless(txpacket == 4);
      check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
      check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

      check_pkt(p, 12, ipproto, sizeof(ipproto)); /* eth level proto: ip */

      check_pkt(p, 42, bootp_start, sizeof(bootp_start));

      check_pkt(p, 53, ipaddrs, sizeof(ipaddrs));

      check_pkt(p, 70, netif->hwaddr, 6); /* mac addr inside bootp */

      check_pkt(p, 278, magic_cookie, sizeof(magic_cookie));

      check_pkt_fuzzy(p, 282, dhcp_nak_opt, sizeof(dhcp_nak_opt)); /* NAK the ack */

      check_pkt_fuzzy(p, 282, requested_ipaddr, sizeof(requested_ipaddr));
      break;
    }

  case TEST_LWIP_DHCP_RELAY:
    switch (txpacket) {
    case 1:
    case 2:
      {
        const u8_t ipproto[] = { 0x08, 0x00 };
        const u8_t bootp_start[] = { 0x01, 0x01, 0x06, 0x00}; /* bootp request, eth, hwaddr len 6, 0 hops */
        const u8_t ipaddrs[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, ipproto, sizeof(ipproto)); /* eth level proto: ip */

        check_pkt(p, 42, bootp_start, sizeof(bootp_start));

        check_pkt(p, 53, ipaddrs, sizeof(ipaddrs));

        check_pkt(p, 70, netif->hwaddr, 6); /* mac addr inside bootp */

        check_pkt(p, 278, magic_cookie, sizeof(magic_cookie));

        /* Check dhcp message type, can be at different positions */
        if (txpacket == 1) {
          u8_t dhcp_discover_opt[] = { 0x35, 0x01, 0x01 };
          check_pkt_fuzzy(p, 282, dhcp_discover_opt, sizeof(dhcp_discover_opt));
        } else if (txpacket == 2) {
          u8_t dhcp_request_opt[] = { 0x35, 0x01, 0x03 };
          u8_t requested_ipaddr[] = { 0x32, 0x04, 0x4f, 0x8a, 0x33, 0x05 }; /* Ask for offered IP */

          check_pkt_fuzzy(p, 282, dhcp_request_opt, sizeof(dhcp_request_opt));
          check_pkt_fuzzy(p, 282, requested_ipaddr, sizeof(requested_ipaddr));
        }
        break;
      }
    case 3:
#if DHCP_TEST_NUM_ARP_FRAMES > 0
    case 4:
#if DHCP_TEST_NUM_ARP_FRAMES > 1
    case 5:
#if DHCP_TEST_NUM_ARP_FRAMES > 2
    case 6:
#if DHCP_TEST_NUM_ARP_FRAMES > 3
    case 7:
#if DHCP_TEST_NUM_ARP_FRAMES > 4
    case 8:
#endif
#endif
#endif
#endif
      {
        const u8_t arpproto[] = { 0x08, 0x06 };

        check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, arpproto, sizeof(arpproto)); /* eth level proto: ip */
        break;
      }
#endif
    case 4 + DHCP_TEST_NUM_ARP_FRAMES:
      {
        const u8_t fake_arp[6] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xab };
        const u8_t ipproto[] = { 0x08, 0x00 };
        const u8_t bootp_start[] = { 0x01, 0x01, 0x06, 0x00}; /* bootp request, eth, hwaddr len 6, 0 hops */
        const u8_t ipaddrs[] = { 0x00, 0x4f, 0x8a, 0x33, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        const u8_t dhcp_request_opt[] = { 0x35, 0x01, 0x03 };

        check_pkt(p, 0, fake_arp, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, ipproto, sizeof(ipproto)); /* eth level proto: ip */

        check_pkt(p, 42, bootp_start, sizeof(bootp_start));

        check_pkt(p, 53, ipaddrs, sizeof(ipaddrs));

        check_pkt(p, 70, netif->hwaddr, 6); /* mac addr inside bootp */

        check_pkt(p, 278, magic_cookie, sizeof(magic_cookie));

        /* Check dhcp message type, can be at different positions */
        check_pkt_fuzzy(p, 282, dhcp_request_opt, sizeof(dhcp_request_opt));
        break;
      }
    default:
      fail();
      break;
    }
    break;

  default:
    break;
  }

  return ERR_OK;
}
/* Context-After
 * /*
 *  * Test basic happy flow DHCP session.
 *  * Validate that xid is checked.
 *  */
 * START_TEST(test_dhcp)
 * {
 *   ip4_addr_t addr;
 *   ip4_addr_t netmask;
 *   ip4_addr_t gw;
 *   int i;
 *   u32_t xid;
 *   LWIP_UNUSED_ARG(_i);
 * 
 *   tcase = TEST_LWIP_DHCP;
 *   setdebug(0);
 * 
 *   IP4_ADDR(&addr, 0, 0, 0, 0);
 *   IP4_ADDR(&netmask, 0, 0, 0, 0);
 *   IP4_ADDR(&gw, 0, 0, 0, 0);
 */
