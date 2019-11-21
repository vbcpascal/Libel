/**
 * @file sdp.h
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-10-19
 *
 * @brief Self Destruct Protocol: An Distance Vector Routing protocol. Read
 * documents for more details.
 *
 */

#ifndef SDP_H_
#define SDP_H_

#include <array>
#include <map>

#include "device.h"
#include "ether.h"
#include "massert.h"
#include "type.h"

#define MAX_SDP_DATA_LEN 255
#define SDP_METRIC_NODEL -1  // will never be deleted from routing table
#define SDP_METRIC_DIE -2
#define SDP_METRIC_TIMEOUT 2

#ifndef NDEBUG
#define ROUTE_LOOP_INTERVAL 10
#else
#define ROUTE_LOOP_INTERVAL 30
#endif

#define DEFINE_SDPPACKET(p, len)                              \
  std::vector<uint8_t> p_##data(sizeof(SDPPacket) + len * 8); \
  auto& p = *reinterpret_cast<SDPPacket*>(p_##data.data())

#define SDPP_SIZE(len) (8 + 8 * len)

namespace Route {

/**
 * @brief Routing item in routing table
 *
 */
class RouteItem {
 public:
  ip_addr ipPrefix;
  ip_addr subNetMask;
  bool isDev;  // or via
  mutable Device::DevicePtr dev;
  mutable MAC::MacAddr nextHopMac;
  mutable int dist;
  mutable int metric;

  RouteItem() = default;
  RouteItem(const ip_addr& _ip, const ip_addr& _mask,
            const Device::DevicePtr& _dev, const MAC::MacAddr& _mac, int _dist,
            bool _isDev, int _metric);

  /**
   * @brief Whether the item can handle an ip address
   *
   * @param ip ip
   * @return true can
   * @return false cannot
   */
  bool haveIp(const ip_addr& ip) const;
};

/**
 * @brief Compare two `RouteItem` according to prefix length
 *
 */
bool operator<(const RouteItem& rl, const RouteItem& rr);

/**
 * @brief Get prefix length from mask
 *
 * @param mask subnet mask
 * @return int prefix length
 */
int maskToPflen(const in_addr& mask);

/**
 * @brief Get prefix length from prefix length
 *
 * @param pflen prefix length
 * @return ip_addr subnet mask
 */
ip_addr pflenToMask(int pflen);

}  // namespace Route

namespace SDP {

/**
 * @brief An SDP item used by SDP routing algorithm
 *
 */
class SDPItem {
 public:
  ip_addr ipPrefix;
  ip_addr subNetMask;
  int dist;
  bool toDel;

  SDPItem() = default;
  SDPItem(const ip_addr& _ip, const ip_addr& _mask, int _dist, bool _del)
      : subNetMask(_mask), dist(_dist), toDel(_del) {
    ipPrefix.s_addr = _ip.s_addr & _mask.s_addr;
  }
};

/**
 * @brief An SDP packet sent and received
 *
 */
struct __attribute__((__packed__)) SDPPacket {
  uint8_t len;
  uint8_t flag;
  u_char mac[6];
  struct __attribute__((__packed__)) {
    in_addr IpPrefix;
    uint8_t pflen;
    uint8_t itemFlag;
    uint16_t dist;
  } data[0];
};

using SDPItemVector = std::vector<SDPItem>;

/**
 * @brief Manage SDP information for routing algorithm
 *
 */
class SDPManager {
 public:
  /**
   * @brief Send a set of SDP items *with Dev* *to Mac*
   *
   * Used only when a new device added into network
   *
   * @param sis SDP items
   * @param flag flag
   * @param withDev send by only device
   * @param toMac send to the specific mac
   */
  void sendSDPPacketsTo(const SDPItemVector& sis, int flag,
                        Device::DevicePtr withDev, MAC::MacAddr toMac);

  /**
   * @brief Send a set of SDP items to neibours for all devices / without
   * specific dev
   *
   * Used most of time
   *
   * @param sis SDP items
   * @param flag flag
   * @param withoutDev send by without this device. nullptr for all devices
   */
  void sendSDPPackets(const SDPItemVector& sis, int flag = 0,
                      Device::DevicePtr withoutDev = nullptr);
};

extern SDPManager sdpMgr;

}  // namespace SDP

#define SDPFLAG_INCREMENT 0x1
#define SDPFLAG_UNFINISH 0x2
#define SDPFLAG_ISNEW 0x4
#define SDPFLAG_VARIFY 0x8

#define SDP_ITEMFLAG_DEL 0x1

namespace Printer {
/**
 * @brief Print a SDP packet
 *
 * @param sdpp SDP packet
 * @param sender whether is sender
 */
void printSDP(const SDP::SDPPacket& sdpp, bool sender = false);
}  // namespace Printer

/*
 * SDP protocol:
 *
 * A protocol of routing, build on Link Layer with EtherType 0x2333. The format
 * of packet is shown below.
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    Length     |     Flag      |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
 * |                                          MAC address          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     Route Address Prefix                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     Pflen     |   Item Flag   |           Distance            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              ...                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Length: the number of routing item (max is 255)
 * Flags: [rrrrvnui]
 *   i Increment  : is an increment routing information.
 *   u Unfinished : have more packet because of length limited
 *   n isNew      : is a new device added into network
 *   v Varify     : is a varify frame
 * Item Flag: [rrrrrrrd]
 *   d Deleted    : deleted from routing table
 * Distance: the distance AFTER adding one for the link
 *
 * When a device added into a network, it will send a packet with all of its
 * subnet ip address (distance 1) and set "isNew" flag, to its neibours.
 *
 * When a device receive a packet with flag "isNew", it should send a packet
 * with full routing information TO the device, with flag "increment" 0.
 *
 * Any time receiving a packet (including situation discussed above), the router
 * should broadcast routing information INCREMENTALLY with flag "increment" 1
 * except src router.
 *
 */

#endif
