#include "sdp.h"

namespace Route {

RouteItem::RouteItem(const ip_addr& _ip, const ip_addr& _mask,
                     const Device::DevicePtr& _dev, const MAC::MacAddr& _mac,
                     int _dist, bool _isDev, int _metric)
    : subNetMask(_mask),
      isDev(_isDev),
      dev(_dev),
      dist(_dist),
      metric(_metric) {
  ipPrefix.s_addr = _ip.s_addr & _mask.s_addr;
  nextHopMac = _mac;
}

bool operator<(const RouteItem& rl, const RouteItem& rr) {
  int ls = maskToPflen(rl.subNetMask), rs = maskToPflen(rr.subNetMask);
  if (ls > rs)
    return true;
  else if (ls < rs)
    return false;

  if (rl.ipPrefix < rr.ipPrefix)
    return true;
  else if (rr.ipPrefix < rl.ipPrefix)
    return false;

  return false;
}

bool RouteItem::haveIp(const ip_addr& ip) const {
  return (ip.s_addr & subNetMask.s_addr) ==
         (ipPrefix.s_addr & subNetMask.s_addr);
}

int maskToPflen(const in_addr& mask) {
  int pflen = 0;
  auto n = mask.s_addr;
  while (n) {
    n = n & (n - 1);
    pflen++;
  }
  return pflen;
}

ip_addr pflenToMask(int pflen) {
  ip_addr ip;
  if (pflen == 0) {
    ip.s_addr = 0;
  } else {
    ip.s_addr = ntohl(~0 << (32 - pflen));
  }
  return ip;
}

}  // namespace Route

namespace SDP {

SDPManager sdpMgr;

void SDPManager::sendSDPPacketsTo(const SDPItemVector& sis, int flag,
                                  Device::DevicePtr withDev,
                                  MAC::MacAddr toMac) {
  int size = sis.size();
  if (size == 0) return;

  ASSERT(size <= MAX_SDP_DATA_LEN, "Too large Routing table: %d", size);
  DEFINE_SDPPACKET(sdppSend, size);
  sdppSend.flag = flag;
  sdppSend.len = size;

  int cnt = 0;
  for (auto& i : sis) {
    sdppSend.data[cnt].IpPrefix = i.ipPrefix;
    sdppSend.data[cnt].pflen = Route::maskToPflen(i.subNetMask);
    sdppSend.data[cnt].dist = i.dist + 1;
    sdppSend.data[cnt].itemFlag |= (i.toDel ? SDP_ITEMFLAG_DEL : 0);
    cnt++;
  }

  withDev->getMAC(sdppSend.mac);
  Printer::printSDP(sdppSend, true);
  Device::deviceMgr.sendFrame(&sdppSend, SDPP_SIZE(size), ETHERTYPE_SDP,
                              toMac.addr, withDev);
}

void SDPManager::sendSDPPackets(const std::vector<SDPItem>& sis, int flag,
                                Device::DevicePtr withoutDev) {
  int size = sis.size();
  if (size == 0) return;

  ASSERT(size <= MAX_SDP_DATA_LEN, "Too large Routing table: %d", size);
  DEFINE_SDPPACKET(sdppSend, size);
  sdppSend.flag = flag;
  sdppSend.len = size;

  int cnt = 0;
  for (auto& i : sis) {
    sdppSend.data[cnt].IpPrefix = i.ipPrefix;
    sdppSend.data[cnt].pflen = Route::maskToPflen(i.subNetMask);
    sdppSend.data[cnt].dist = i.dist + 1;
    sdppSend.data[cnt].itemFlag |= (i.toDel ? SDP_ITEMFLAG_DEL : 0);
    cnt++;
  }

  for (auto& dev : Device::deviceMgr.devices) {
    if (dev == withoutDev) continue;
    dev->getMAC(sdppSend.mac);
    Printer::printSDP(sdppSend, true);
    Device::deviceMgr.sendFrame(&sdppSend, SDPP_SIZE(size), ETHERTYPE_SDP,
                                Ether::broadcastMacAddr, dev);
  }
}
}  // namespace SDP

namespace Printer {
void printSDP(const SDP::SDPPacket& sdpp, bool sender) {
  printf("\033[;1m%s SDP\033[0m len: %d, from %s\n", (sender ? "--" : ">>"),
         sdpp.len, MAC::toString(sdpp.mac).c_str());
  for (int i = 0; i < sdpp.len; ++i) {
    printf(" |  %s/%d\t(dist: %d)\t%s\n", inet_ntoa(sdpp.data[i].IpPrefix),
           sdpp.data[i].pflen, sdpp.data[i].dist,
           sdpp.data[i].itemFlag & SDP_ITEMFLAG_DEL ? "DEL" : "");
  }
}
}  // namespace Printer