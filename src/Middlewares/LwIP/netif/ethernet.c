#include "lwip/opt.h"

#if LWIP_ARP || LWIP_ETHERNET

#include "netif/ethernet.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"
#include "lwip/ip.h"
#include "lwip/snmp.h"

#include <string.h>

#include "netif/ppp/ppp_opts.h"
#if PPPOE_SUPPORT
#include "netif/ppp/pppoe.h"
#endif /* PPPOE_SUPPORT */

const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};
const struct eth_addr ethzero = {{0,0,0,0,0,0}};

/**
 * @ingroup lwip_nosys
 * Process received ethernet frames. Using this function instead of directly
 * calling ip_input and passing ARP frames through etharp in ethernetif_input,
 * the ARP cache is protected from concurrent access.\n
 * Don't call directly, pass to netif_add() and call netif->input().
 *
 * @param p the received packet, p->payload pointing to the ethernet header
 * @param netif the network interface on which the packet was received
 * 
 * @see LWIP_HOOK_UNKNOWN_ETH_PROTOCOL
 * @see ETHARP_SUPPORT_VLAN
 * @see LWIP_HOOK_VLAN_CHECK
 */
err_t
ethernet_input(struct pbuf *p, struct netif *netif)
{
  struct eth_hdr* ethhdr;
  u16_t type;
#if LWIP_ARP || ETHARP_SUPPORT_VLAN || LWIP_IPV6
  s16_t ip_hdr_offset = SIZEOF_ETH_HDR;
#endif /* LWIP_ARP || ETHARP_SUPPORT_VLAN */

  if (p->len <= SIZEOF_ETH_HDR) {
    /* a packet with only an ethernet header (or less) is not valid for us */
    ETHARP_STATS_INC(etharp.proterr);
    ETHARP_STATS_INC(etharp.drop);
    MIB2_STATS_NETIF_INC(netif, ifinerrors);
    goto free_and_return;
  }

  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = (struct eth_hdr *)p->payload;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_input: dest:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", src:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", type:%"X16_F"\r\n",
     (unsigned)ethhdr->dest.addr[0], (unsigned)ethhdr->dest.addr[1], (unsigned)ethhdr->dest.addr[2],
     (unsigned)ethhdr->dest.addr[3], (unsigned)ethhdr->dest.addr[4], (unsigned)ethhdr->dest.addr[5],
     (unsigned)ethhdr->src.addr[0],  (unsigned)ethhdr->src.addr[1],  (unsigned)ethhdr->src.addr[2],
     (unsigned)ethhdr->src.addr[3],  (unsigned)ethhdr->src.addr[4],  (unsigned)ethhdr->src.addr[5],
     lwip_htons(ethhdr->type)));

  type = ethhdr->type;
#if ETHARP_SUPPORT_VLAN
  if (type == PP_HTONS(ETHTYPE_VLAN)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr*)(((char*)ethhdr) + SIZEOF_ETH_HDR);
    if (p->len <= SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) {
      /* a packet with only an ethernet/vlan header (or less) is not valid for us */
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
      MIB2_STATS_NETIF_INC(netif, ifinerrors);
      goto free_and_return;
    }
#if defined(LWIP_HOOK_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) /* if not, allow all VLANs */
#ifdef LWIP_HOOK_VLAN_CHECK
    if (!LWIP_HOOK_VLAN_CHECK(netif, ethhdr, vlan)) {
#elif defined(ETHARP_VLAN_CHECK_FN)
    if (!ETHARP_VLAN_CHECK_FN(ethhdr, vlan)) {
#elif defined(ETHARP_VLAN_CHECK)
    if (VLAN_ID(vlan) != ETHARP_VLAN_CHECK) {
#endif
      /* silently ignore this packet: not for our VLAN */
      pbuf_free(p);
      return ERR_OK;
    }
#endif /* defined(LWIP_HOOK_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK) || defined(ETHARP_VLAN_CHECK_FN) */
    type = vlan->tpid;
    ip_hdr_offset = SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR;
  }
#endif /* ETHARP_SUPPORT_VLAN */

#if LWIP_ARP_FILTER_NETIF
  netif = LWIP_ARP_FILTER_NETIF_FN(p, netif, lwip_htons(type));
#endif /* LWIP_ARP_FILTER_NETIF*/

  if (ethhdr->dest.addr[0] & 1) {
    /* this might be a multicast or broadcast packet */
    if (ethhdr->dest.addr[0] == LL_IP4_MULTICAST_ADDR_0) {
#if LWIP_IPV4
      if ((ethhdr->dest.addr[1] == LL_IP4_MULTICAST_ADDR_1) &&
          (ethhdr->dest.addr[2] == LL_IP4_MULTICAST_ADDR_2)) {
        /* mark the pbuf as link-layer multicast */
        p->flags |= PBUF_FLAG_LLMCAST;
      }
#endif /* LWIP_IPV4 */
    }
#if LWIP_IPV6
    else if ((ethhdr->dest.addr[0] == LL_IP6_MULTICAST_ADDR_0) &&
             (ethhdr->dest.addr[1] == LL_IP6_MULTICAST_ADDR_1)) {
        /* mark the pbuf as link-layer multicast */
        p->flags |= PBUF_FLAG_LLMCAST;
    }
#endif /* LWIP_IPV6 */
    else if (eth_addr_cmp(&ethhdr->dest, &ethbroadcast)) {
      /* mark the pbuf as link-layer broadcast */
      p->flags |= PBUF_FLAG_LLBCAST;
    }
  }

  switch (type) {
#if LWIP_IPV4 && LWIP_ARP
    /* IP packet? */
    case PP_HTONS(ETHTYPE_IP):
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        goto free_and_return;
      }
      /* skip Ethernet header */
      if ((p->len < ip_hdr_offset) || pbuf_header(p, (s16_t)-ip_hdr_offset)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
          ("ethernet_input: IPv4 packet dropped, too short (%"S16_F"/%"S16_F")\r\n",
          p->tot_len, ip_hdr_offset));
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("Can't move over header in packet"));
        goto free_and_return;
      } else {
        /* pass to IP layer */
        ip4_input(p, netif);
      }
      break;

    case PP_HTONS(ETHTYPE_ARP):
      if (!(netif->flags & NETIF_FLAG_ETHARP)) {
        goto free_and_return;
      }
      /* skip Ethernet header */
      if ((p->len < ip_hdr_offset) || pbuf_header(p, (s16_t)-ip_hdr_offset)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
          ("ethernet_input: ARP response packet dropped, too short (%"S16_F"/%"S16_F")\r\n",
          p->tot_len, ip_hdr_offset));
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("Can't move over header in packet"));
        ETHARP_STATS_INC(etharp.lenerr);
        ETHARP_STATS_INC(etharp.drop);
        goto free_and_return;
      } else {
        /* pass p to ARP module */
        etharp_input(p, netif);
      }
      break;
#endif /* LWIP_IPV4 && LWIP_ARP */
#if PPPOE_SUPPORT
    case PP_HTONS(ETHTYPE_PPPOEDISC): /* PPP Over Ethernet Discovery Stage */
      pppoe_disc_input(netif, p);
      break;

    case PP_HTONS(ETHTYPE_PPPOE): /* PPP Over Ethernet Session Stage */
      pppoe_data_input(netif, p);
      break;
#endif /* PPPOE_SUPPORT */

#if LWIP_IPV6
    case PP_HTONS(ETHTYPE_IPV6): /* IPv6 */
      /* skip Ethernet header */
      if ((p->len < ip_hdr_offset) || pbuf_header(p, (s16_t)-ip_hdr_offset)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
          ("ethernet_input: IPv6 packet dropped, too short (%"S16_F"/%"S16_F")\r\n",
          p->tot_len, ip_hdr_offset));
        goto free_and_return;
      } else {
        /* pass to IPv6 layer */
        ip6_input(p, netif);
      }
      break;
#endif /* LWIP_IPV6 */

    default:
#ifdef LWIP_HOOK_UNKNOWN_ETH_PROTOCOL
      if(LWIP_HOOK_UNKNOWN_ETH_PROTOCOL(p, netif) == ERR_OK) {
        break;
      }
#endif
      ETHARP_STATS_INC(etharp.proterr);
      ETHARP_STATS_INC(etharp.drop);
      MIB2_STATS_NETIF_INC(netif, ifinunknownprotos);
      goto free_and_return;
  }

  /* This means the pbuf is freed or consumed,
     so the caller doesn't have to free it again */
  return ERR_OK;

free_and_return:
  pbuf_free(p);
  return ERR_OK;
}

/**
 * @ingroup ethernet
 * Send an ethernet packet on the network using netif->linkoutput().
 * The ethernet header is filled in before sending.
 *
 * @see LWIP_HOOK_VLAN_SET
 *
 * @param netif the lwIP network interface on which to send the packet
 * @param p the packet to send. pbuf layer must be @ref PBUF_LINK.
 * @param src the source MAC address to be copied into the ethernet header
 * @param dst the destination MAC address to be copied into the ethernet header
 * @param eth_type ethernet type (@ref eth_type)
 * @return ERR_OK if the packet was sent, any other err_t on failure
 */
err_t
ethernet_output(struct netif* netif, struct pbuf* p,
                const struct eth_addr* src, const struct eth_addr* dst,
                u16_t eth_type)
{
  struct eth_hdr* ethhdr;
  u16_t eth_type_be = lwip_htons(eth_type);

#if ETHARP_SUPPORT_VLAN && defined(LWIP_HOOK_VLAN_SET)
  s32_t vlan_prio_vid = LWIP_HOOK_VLAN_SET(netif, p, src, dst, eth_type);
  if (vlan_prio_vid >= 0) {
    struct eth_vlan_hdr* vlanhdr;

    LWIP_ASSERT("prio_vid must be <= 0xFFFF", vlan_prio_vid <= 0xFFFF);

    if (pbuf_header(p, SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) != 0) {
      goto pbuf_header_failed;
    }
    vlanhdr = (struct eth_vlan_hdr*)(((u8_t*)p->payload) + SIZEOF_ETH_HDR);
    vlanhdr->tpid     = eth_type_be;
    vlanhdr->prio_vid = lwip_htons((u16_t)vlan_prio_vid);

    eth_type_be = PP_HTONS(ETHTYPE_VLAN);
  } else
#endif /* ETHARP_SUPPORT_VLAN && defined(LWIP_HOOK_VLAN_SET) */
  {
    if (pbuf_header(p, SIZEOF_ETH_HDR) != 0) {
      goto pbuf_header_failed;
    }
  }

  ethhdr = (struct eth_hdr*)p->payload;
  ethhdr->type = eth_type_be;
  ETHADDR32_COPY(&ethhdr->dest, dst);
  ETHADDR16_COPY(&ethhdr->src,  src);

  LWIP_ASSERT("netif->hwaddr_len must be 6 for ethernet_output!",
    (netif->hwaddr_len == ETH_HWADDR_LEN));
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_output: sending packet %p\r\n", (void *)p));

  /* send the packet */
  return netif->linkoutput(netif, p, netif->state);

pbuf_header_failed:
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
    ("ethernet_output: could not allocate room for header.\r\n"));
  LINK_STATS_INC(link.lenerr);
  return ERR_BUF;
}


err_t
	ethernet_output_goose(struct netif* netif, struct pbuf* p, 
		const struct eth_addr* src, const struct eth_addr* dst, uint8_t priority,
		uint16_t vlanId, uint16_t appId) {
		
  struct goose_hdr* ethhdr;
	u16_t data_len = p->len;
  //u16_t eth_type_be = lwip_htons(ETHTYPE_IEC61850);
	//u16_t eth_tdip = lwip_htons(ETHTYPE_VLAN);
	//uint8_t tci1 = (priority << 5) + (vlanId / 256);
	//uint8_t tci2 = vlanId % 256;
	//u16_t vlan_prio_vid = (u16_t)(tci1 << 8) || (u16_t)tci2;
	// здесь надо как-то подругому сделать
	//u16_t vlan_prio_vid = vlanId;

	if (pbuf_header2(p, SIZEOF_GOOSE_HDR) != 0) {
      goto pbuf_header_failed;
    }

  ethhdr = (struct goose_hdr*)p->payload;
	ETHADDR32_COPY(&ethhdr->dest, dst);
	ETHADDR16_COPY(&ethhdr->src,  src);
	ethhdr->tpid = lwip_htons(ETHTYPE_VLAN);
	ethhdr->tci = lwip_htons((u16_t)vlanId);
  ethhdr->type = lwip_htons(ETHTYPE_IEC61850);
	ethhdr->appid = 0;
	ethhdr->lenght = lwip_htons(data_len);
	ethhdr->res1 = 0;
	ethhdr->res2 = 0;
  

  LWIP_ASSERT("netif->hwaddr_len must be 6 for ethernet_output_goose!",
    (netif->hwaddr_len == ETH_HWADDR_LEN));
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_output_goose: sending packet %p\r\n", (void *)p));

  /* send the packet */
  return netif->linkoutput(netif, p, netif->state);

pbuf_header_failed:
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
    ("ethernet_output_goose: could not allocate room for header.\r\n"));
  LINK_STATS_INC(link.lenerr);
  return ERR_BUF;
}


err_t
	ethernet_output_raw(struct netif* netif, struct pbuf* p, 
		const struct eth_addr* src, const struct eth_addr* dst, u16_t eth_type) {
  
	struct eth_hdr* ethhdr;
  u16_t eth_type_be = lwip_htons(eth_type);

  ethhdr = (struct eth_hdr*)p->payload;
  ethhdr->type = eth_type_be;
  ETHADDR32_COPY(&ethhdr->dest, dst);
  ETHADDR16_COPY(&ethhdr->src,  src);

  LWIP_ASSERT("netif->hwaddr_len must be 6 for ethernet_output!",
    (netif->hwaddr_len == ETH_HWADDR_LEN));
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_output: sending packet %p\r\n", (void *)p));

  /* send the packet */
  return netif->linkoutput(netif, p, netif->state);
}

#endif /* LWIP_ARP || LWIP_ETHERNET */
