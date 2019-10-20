#include "router.h"

namespace SDP {

int sdpCallBack(const void* buf, int len, DeviceId id) {
  int pLen = ((int*)buf)[0];
  DEFINE_SDPPACKET(sdpp, pLen);
  memcpy(&sdpp, buf, sizeof(sdpp));
  Printer::printSDP(sdpp);

  SDPItemVector sis;

  for (int i = 0; i < pLen; ++i) {
    sis.push_back(SDPItem(sdpp.data[i].IpPrefix,
                          Route::slashToMask(sdpp.data[i].slash),
                          sdpp.data[i].dist));
  }

  Route::router.update(sis, MAC::MacAddr(sdpp.mac),
                       Device::deviceMgr.getDevicePtr(id));

  // receive an "isnew"
  if (sdpp.flag & SDPFLAG_ISNEW) {
    int size = Route::router.table.size();
    DEFINE_SDPPACKET(sdpp, size);
    sdpp.flag = 0;
    sdpp.len = size;

    int cnt = 0;
    for (auto& i : Route::router.table) {
      sdpp.data[cnt].IpPrefix = i.ipPrefix;
      sdpp.data[cnt].slash = Route::maskToSlash(i.subNetMask);
      sdpp.data[cnt].dist = i.dist + 1;
      cnt++;
    }

    Printer::printSDP(sdpp);
    Device::deviceMgr.sendFrame(&sdpp, sizeof(sdpp), ETHERTYPE_SDP, sdpp.mac,
                                id);
  }

  return 0;
}

}  // namespace SDP

namespace Route {

Router router;

std::pair<Device::DevicePtr, MAC::MacAddr> Router::lookup(const ip_addr& ip) {
  Device::DevicePtr dev = nullptr;
  MAC::MacAddr mac;
  for (auto& d : table) {
    if (d.haveIp(ip)) {
      dev = d.dev;
      mac = d.nextHopMac;
      break;
    }
  }
  return std::make_pair(dev, mac);
}

int Router::setTable(const in_addr& dst, const in_addr& mask,
                     const MAC::MacAddr& nextHopMac,
                     const Device::DevicePtr& dev) {
  RouteItem r(dst, mask, dev, nextHopMac);
  table.insert(r);
  return 0;
}

int Router::setItem(const RouteItem& ri) {
  for (auto& i : table) {
    if (i.subNetMask == ri.subNetMask) {
      if (i.haveIp(ri.ipPrefix) && ri.haveIp(i.ipPrefix)) {
        table.erase(i);
        table.insert(ri);
        return 1;
      }
    }
  }
  table.insert(ri);
  return 0;
}

void Router::init() {
  SDP::SDPItemVector sis;
  for (auto& d : Device::deviceMgr.devices) {
    SDP::SDPItem s(d->getIp(), d->getSubnetMask(), 0);
    sis.push_back(s);
  }
  SDP::sdpMgr.sendSDPPackets(sis, SDPFLAG_ISNEW);
}

void Router::update(const SDP::SDPItemVector& sis, const MAC::MacAddr mac,
                    const Device::DevicePtr dev) {
  SDP::SDPItemVector updateSis;

  for (auto& si : sis) {
    auto prefix = si.ipPrefix;
    auto mask = si.subNetMask;
    int d = si.dist;

    bool handled = false;
    // already exist
    for (auto& ri : table) {
      if (prefix == ri.ipPrefix && mask == ri.subNetMask && d < ri.dist) {
        ri.nextHopMac = mac;
        ri.dev = dev;
        updateSis.push_back(SDP::SDPItem(prefix, mask, d));
        handled = true;
        break;
      }
    }
    if (handled) continue;

    // not my subnet
    for (auto& d : Device::deviceMgr.devices) {
      if (prefix.s_addr == (d->getIp().s_addr & d->getSubnetMask().s_addr) &&
          mask == d->getSubnetMask())
        handled = true;
    }
    if (handled) continue;

    // then add to the router
    table.insert(RouteItem(prefix, mask, dev, mac, d));
    updateSis.push_back(SDP::SDPItem(prefix, mask, d));
  }

  Printer::printRouteTable();

  SDP::sdpMgr.sendSDPPackets(updateSis, SDPFLAG_INCREMENT, dev);
}

}  // namespace Route

namespace Printer {
void printRouteItem(const Route::RouteItem& r) {
  printf("  %s\t%d\t%s\t%s\n", inet_ntoa(r.ipPrefix),
         Route::maskToSlash(r.subNetMask),
         MAC::toString(r.nextHopMac.addr).c_str(), r.dev->getName().c_str());
}
void printRouteTable() {
  printf(
      "\n====================== Routing Table "
      "======================\n");
  for (auto& r : Route::router.table) {
    Printer::printRouteItem(r);
  }
  printf(
      "====================================="
      "======================\n\n");
}
}  // namespace Printer