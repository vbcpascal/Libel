/**
 * @file sdp.h
 * @author guanzhichao
 * @brief Self Destruct Protocol: An Distance Vector Routing protocol
 * @version 0.1
 * @date 2019-10-19
 *
 * @copyright Copyright (c) 2019
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

#define DEFINE_SDPPACKET(p, len)                              \
  std::vector<uint8_t> p_##data(sizeof(SDPPacket) + len * 8); \
  auto& p = *reinterpret_cast<SDPPacket*>(p_##data.data())

namespace Route {

class RouteItem {
 public:
  ip_addr ipPrefix;
  ip_addr subNetMask;
  mutable Device::DevicePtr dev;
  mutable MAC::MacAddr nextHopMac;
  int dist;

  RouteItem() = default;
  RouteItem(const ip_addr& _ip, const ip_addr& _mask,
            const Device::DevicePtr& _d, const MAC::MacAddr& _m, int _dist = 0);
  bool haveIp(const ip_addr& ip) const;
};

bool operator<(const RouteItem& rl, const RouteItem& rr);

int maskToSlash(const in_addr& mask);
ip_addr slashToMask(int slash);
}  // namespace Route

namespace SDP {

class SDPItem {
 public:
  ip_addr ipPrefix;
  ip_addr subNetMask;
  int dist;

  SDPItem() = default;
  SDPItem(const ip_addr& _ip, const ip_addr& _mask, int _dist = 0)
      : subNetMask(_mask), dist(_dist) {
    ipPrefix.s_addr = _ip.s_addr & _mask.s_addr;
  }
};

struct __attribute__((__packed__)) SDPPacket {
  uint8_t len;
  uint8_t flag;
  u_char mac[6];
  struct __attribute__((__packed__)) {
    in_addr IpPrefix;
    uint16_t slash;
    uint16_t dist;
  } data[0];
};

using SDPItemVector = std::vector<SDPItem>;

class SDPManager {
 public:
  void sendSDPPackets(const SDPItemVector& sis, int flag = 0,
                      Device::DevicePtr withoutDev = nullptr);
};

extern SDPManager sdpMgr;

}  // namespace SDP

#define SDPFLAG_INCREMENT 0x1
#define SDPFLAG_UNFINISH 0x2
#define SDPFLAG_ISNEW 0x4

namespace Printer {
void printSDP(const SDP::SDPPacket& sdpp, bool sender = false);
}

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
 * |             Slash             |           Distance            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              ...                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Length: the number of routing item (max is 255)
 * Flags: [rrrrrnui]
 *   i Increment  : is an increment routing information.
 *   u Unfinished : have more packet because of length limited
 *   n isNew      : is a new device added into network
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
 *
 */

#endif
