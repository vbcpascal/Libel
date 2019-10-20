#include "sdp.h"

namespace Route {

RouteItem::RouteItem(const ip_addr& _ip, const ip_addr& _mask,
                     const Device::DevicePtr& _d, const MAC::MacAddr& _m,
                     int _dist)
    : subNetMask(_mask), dev(_d), dist(_dist) {
  ipPrefix.s_addr = _ip.s_addr & _mask.s_addr;
  nextHopMac = _m;
}

bool operator<(const RouteItem& rl, const RouteItem& rr) {
  int ls = maskToSlash(rl.subNetMask), rs = maskToSlash(rr.subNetMask);
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

int maskToSlash(const in_addr& mask) {
  int slash = 0;
  auto n = mask.s_addr;
  while (n) {
    n = n & (n - 1);
    slash++;
  }
  return slash;
}

ip_addr slashToMask(int slash) {
  ip_addr ip;
  if (slash == 0) {
    ip.s_addr = 0;
  } else {
    ip.s_addr = ntohl(~0 << (32 - slash));
  }
  return ip;
}

}  // namespace Route

namespace SDP {

SDPManager sdpMgr;

void SDPManager::sendSDPPackets(const std::vector<SDPItem>& sis, int flag,
                                Device::DevicePtr withoutDev) {
  int size = sis.size();
  ASSERT(size > MAX_SDP_DATA_LEN, "Too large Routing table: %d", size);
  DEFINE_SDPPACKET(sdpp, size);
  sdpp.flag = flag;
  sdpp.len = size;

  int cnt = 0;
  for (auto& i : sis) {
    sdpp.data[cnt].IpPrefix = i.ipPrefix;
    sdpp.data[cnt].slash = Route::maskToSlash(i.subNetMask);
    sdpp.data[cnt].dist = i.dist + 1;
    cnt++;
  }

  for (auto& dev : Device::deviceMgr.devices) {
    if (dev == withoutDev) continue;
    dev->getMAC(sdpp.mac);
    Printer::printSDP(sdpp);
    Device::deviceMgr.sendFrame(&sdpp, sizeof(sdpp), ETHERTYPE_SDP,
                                Ether::broadcastMacAddr, dev);
  }
}
}  // namespace SDP

namespace Printer {
void printSDP(const SDP::SDPPacket& sdpp, bool sender) {
  printf("\033[;1m%s SDP\033[0m len: %d, from %s\n", (sender ? "--" : ">>"),
         sdpp.len, MAC::toString(sdpp.mac).c_str());
  for (int i = 0; i < sdpp.len; ++i) {
    printf(" |  %s/%d\t(dist: %d)\n", inet_ntoa(sdpp.data[i].IpPrefix),
           sdpp.data[i].slash, sdpp.data[i].dist);
  }
}
}  // namespace Printer