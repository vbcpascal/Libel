/**
 * @file router.h
 * @author guanzhichao
 * @brief Library supporting Router
 * @version 0.1
 * @date 2019-10-18
 *
 */

#ifndef ROUTER_H_
#define ROUTER_H_

#include <set>

#include "device.h"

namespace Route {

class RouteItem {
 public:
  ip_addr ipPrefix;
  ip_addr subNetMask;
  Device::DevicePtr dev;
  MAC::MacAddr nextHopMac;

  RouteItem() = default;
  RouteItem(const ip_addr& _ip, const ip_addr& _mask,
            const Device::DevicePtr& _d, const MAC::MacAddr& _m);
  bool haveIp(const ip_addr& ip) const;
};

bool operator<(const RouteItem& rl, const RouteItem& rr);

using RoutingTable = std::set<RouteItem>;

class Router {
 public:
  RoutingTable table;

  std::pair<Device::DevicePtr, MAC::MacAddr> lookup(const ip_addr& ip);
  int setTable(const in_addr& dst, const in_addr& mask,
               const MAC::MacAddr& nextHopMac, const Device::DevicePtr& dev);
};

extern Router router;

}  // namespace Route

namespace Printer {
void printRouteItem(const Route::RouteItem& r);
void printRouteTable();
}  // namespace Printer

#endif
