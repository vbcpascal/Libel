#include "router.h"

#include "device.h"

namespace {
int maskToSlash(const in_addr& mask) {
  int slash = 0;
  auto n = mask.s_addr;
  while (n) {
    n = n & (n - 1);
    slash++;
  }
  return slash;
}
}  // namespace

namespace Route {

Router router;

RouteItem::RouteItem(const ip_addr& _ip, const ip_addr& _mask,
                     const Device::DevicePtr& _d, const MAC::MacAddr& _m)
    : ipPrefix(_ip), subNetMask(_mask), dev(_d) {
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

  if (rl.dev < rr.dev)
    return true;
  else
    return false;
}

bool RouteItem::haveIp(const ip_addr& ip) const {
  return (ip.s_addr & subNetMask.s_addr) ==
         (ipPrefix.s_addr & subNetMask.s_addr);
}

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

}  // namespace Route

namespace Printer {
void printRouteItem(const Route::RouteItem& r) {
  printf("  %s\t%d\t%s\t%s\n", inet_ntoa(r.ipPrefix), maskToSlash(r.subNetMask),
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