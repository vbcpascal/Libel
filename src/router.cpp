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

Route::Route(const ip_addr& _ip, int _s, const Device::DevicePtr& _d,
             const MAC::macAddr& _m)
    : ipPrefix(_ip), slash(_s), dev(_d) {
  nextHopMac = _m;
}

bool operator<(const Route& rl, const Route& rr) {
  if (rl.slash > rr.slash) return true;
  if (rl.ipPrefix < rr.ipPrefix) return true;
  if (rl.dev < rr.dev) return true;
  return false;
}

bool Route::haveIp(const ip_addr& ip) {
  return !((ip.s_addr ^ ipPrefix.s_addr) >> (32 - slash));
}

std::pair<Device::DevicePtr, MAC::macAddr> Router::lookup(const ip_addr& ip) {
  Device::DevicePtr dev = nullptr;
  MAC::macAddr mac;
  for (auto d : table) {
    if (d.haveIp(ip)) {
      dev = d.dev;
      mac = d.nextHopMac;
      break;
    }
  }
  return std::make_pair(dev, mac);
}

int Router::setTable(const in_addr& dst, const in_addr& mask,
                     const MAC::macAddr& nextHopMac,
                     const Device::DevicePtr& dev) {
  Route r(dst, maskToSlash(mask), dev, nextHopMac);
  table.insert(r);
  return 0;
}

}  // namespace Route