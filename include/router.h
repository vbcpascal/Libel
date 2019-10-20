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
#include "sdp.h"

namespace SDP {
int sdpCallBack(const void* buf, int len, DeviceId id);
}

namespace Route {

using RoutingTable = std::set<RouteItem>;

class Router {
 public:
  RoutingTable table;

  std::pair<Device::DevicePtr, MAC::MacAddr> lookup(const ip_addr& ip);
  int setTable(const in_addr& dst, const in_addr& mask,
               const MAC::MacAddr& nextHopMac, const Device::DevicePtr& dev);
  int setItem(const RouteItem& ri);
  void init();
  void update(const SDP::SDPItemVector& sis, const MAC::MacAddr mac,
              const Device::DevicePtr dev);
};

extern Router router;

}  // namespace Route

namespace Printer {
void printRouteItem(const Route::RouteItem& r);
void printRouteTable();
}  // namespace Printer

#endif
