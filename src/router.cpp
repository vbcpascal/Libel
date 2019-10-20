#include "router.h"

namespace SDP {

int sdpCallBack(const void* buf, int len, DeviceId id) {
  uint8_t pLen = ((uint8_t*)buf)[0];
  DEFINE_SDPPACKET(sdppGet, pLen);
  memcpy(&sdppGet, buf, SDPP_SIZE(pLen));
  Printer::printSDP(sdppGet);

  SDPItemVector sis;

  // get the SDP items
  for (int i = 0; i < pLen; ++i) {
    sis.push_back(SDPItem(sdppGet.data[i].IpPrefix,
                          Route::slashToMask(sdppGet.data[i].slash),
                          sdppGet.data[i].dist));
  }

  // update routing table
  Route::router.update(sis, MAC::MacAddr(sdppGet.mac),
                       Device::deviceMgr.getDevicePtr(id));

  // if receive an frame with "isnew" flag
  if (sdppGet.flag & SDPFLAG_ISNEW) {
    auto dev = Device::deviceMgr.getDevicePtr(id);
    Route::router.sendRoutingTable(0, dev, sdppGet.mac);
  }

  return 0;
}

}  // namespace SDP

namespace Route {

Router router;

RouteItem Router::lookup(const ip_addr& ip) {
  RouteItem resRi;
  Device::DevicePtr dev = nullptr;
  MAC::MacAddr mac;
  resRi.ipPrefix.s_addr = 0;
  for (auto& ri : table) {
    if (ri.haveIp(ip)) {
      resRi = ri;
      break;
    }
  }
  return resRi;
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
  for (auto& d : Device::deviceMgr.devices) {
    table.insert(
        RouteItem(d->getIp(), d->getSubnetMask(), d, d->getMAC(), 0, true));
  }
  Printer::printRouteTable();
  sendRoutingTable(SDPFLAG_ISNEW, {}, {});
}

void Router::sendRoutingTable(int flag,
                              std::optional<Device::DevicePtr> withDev,
                              std::optional<MAC::MacAddr> toMac) {
  SDP::SDPItemVector sis;

  for (auto& ri : table) {
    sis.push_back(SDP::SDPItem(ri.ipPrefix, ri.subNetMask, ri.dist));
  }

  if (withDev) {
    MAC::MacAddr mac = toMac.value_or(MAC::MacAddr(Ether::broadcastMacAddr));
    SDP::sdpMgr.sendSDPPacketsTo(sis, flag, withDev.value(), mac);
  } else {
    SDP::sdpMgr.sendSDPPackets(sis, flag, nullptr);
  }
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
      if (prefix == ri.ipPrefix && mask == ri.subNetMask) {
        if (d < ri.dist) {
          ri.nextHopMac = mac;
          ri.dev = dev;
          updateSis.push_back(SDP::SDPItem(prefix, mask, d));
        }
        handled = true;
        break;
      }
    }
    if (handled) continue;

    // else add to the router
    table.insert(RouteItem(prefix, mask, dev, mac, d, false));
    updateSis.push_back(SDP::SDPItem(prefix, mask, d));
  }

  LOG_INFO("Update routing items: %zu", updateSis.size());
  Printer::printRouteTable();
  SDP::sdpMgr.sendSDPPackets(updateSis, SDPFLAG_INCREMENT, dev);
}

}  // namespace Route

namespace Printer {
void printRouteItem(const Route::RouteItem& r) {
  printf("  %s/%d  \t%s\t%s\t%s(%d)\n", inet_ntoa(r.ipPrefix),
         Route::maskToSlash(r.subNetMask),
         MAC::toString(r.nextHopMac.addr).c_str(), r.dev->getName().c_str(),
         (r.isDev ? "dev" : "via"), r.dist);
}
void printRouteTable() {
  printf(
      "\n========================= Routing Table "
      "=========================\n");
  for (auto& r : Route::router.table) {
    Printer::printRouteItem(r);
  }
  printf(
      "==========================================="
      "======================\n\n");
}
}  // namespace Printer