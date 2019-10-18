#ifndef ROUTER_H_
#define ROUTER_H_

#include <set>

#include "device.h"

namespace Route {

class Route {
 public:
  ip_addr ipPrefix;
  int slash;
  Device::DevicePtr dev;
  MAC::macAddr nextHopMac;

  Route() = default;
  Route(const ip_addr& _ip, int _s, const Device::DevicePtr& _d,
        const MAC::macAddr& _m);
  bool haveIp(const ip_addr& ip) const;
};

bool operator<(const Route& rl, const Route& rr);

using RoutingTable = std::set<Route>;

class Router {
 public:
  RoutingTable table;

  std::pair<Device::DevicePtr, MAC::macAddr> lookup(const ip_addr& ip);
  int setTable(const in_addr& dst, const in_addr& mask,
               const MAC::macAddr& nextHopMac, const Device::DevicePtr& dev);
};

extern Router router;

}  // namespace Route

namespace Printer {
void printRouteItem(const Route::Route& r);
void printRouteTable();
}  // namespace Printer

#endif
