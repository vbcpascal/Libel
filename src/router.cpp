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
    auto data = sdppGet.data[i];
    sis.push_back(SDPItem(data.IpPrefix, Route::pflenToMask(data.pflen),
                          data.dist,
                          (data.itemFlag & SDP_ITEMFLAG_DEL ? true : false)));
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

int Router::addItem(const RouteItem& ri) {
  SDP::SDPItemVector sis;

  sis.push_back(SDP::SDPItem(ri.ipPrefix, ri.subNetMask, 1, -1));
  update(sis, ri.nextHopMac, ri.dev);

  return 0;
}

void Router::init() {
  for (auto& d : Device::deviceMgr.devices) {
    table.insert(RouteItem(d->getIp(), d->getSubnetMask(), d, d->getMAC(), 0,
                           true, SDP_METRIC_NODEL));
  }
  Printer::printRouteTable();
  sendRoutingTable(SDPFLAG_ISNEW, {}, {});
  loopThread = std::thread([&]() { this->routerWorkingLoop(); });
}

void Router::sendRoutingTable(int flag,
                              std::optional<Device::DevicePtr> withDev,
                              std::optional<MAC::MacAddr> toMac) {
  SDP::SDPItemVector sis;

  for (auto& ri : table) {
    if ((ri.metric >= 0 && ri.metric < SDP_METRIC_TIMEOUT) ||
        ri.metric == SDP_METRIC_NODEL)
      sis.push_back(SDP::SDPItem(ri.ipPrefix, ri.subNetMask, ri.dist, false));
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
    int dist = si.dist;
    bool del = si.toDel;

    bool handled = false;
    // already exist
    for (auto& ri : table) {
      if (prefix == ri.ipPrefix && mask == ri.subNetMask) {
        // from the same device: update metric
        if (mac == ri.nextHopMac) {
          if (del) {
            ri.metric = SDP_METRIC_TIMEOUT;
            updateSis.push_back(SDP::SDPItem(prefix, mask, dist, true));
          } else {
            ri.metric = 0;
          }
        }
        // TIMEOUT yet: do nothing
        else if (ri.metric == SDP_METRIC_TIMEOUT) {
          // nothing
        }
        // from a better device: update all
        else if (dist < ri.dist && !del) {
          ri.nextHopMac = mac;
          ri.dev = dev;
          ri.dist = dist;
          ri.metric = 0;
          updateSis.push_back(SDP::SDPItem(prefix, mask, dist, false));
        }
        handled = true;
        break;
      }
    }
    if (handled) continue;

    // else add to the router
    if (!del) {
      table.insert(RouteItem(prefix, mask, dev, mac, dist, false, 0));
      updateSis.push_back(SDP::SDPItem(prefix, mask, dist, false));
    } else {
      LOG_ERR("Get a Delete Item but not in the routing table.");
    }
  }

  LOG_INFO("Update routing items: %zu", updateSis.size());
  if (updateSis.size() == 0) return;
  Printer::printRouteTable();
  SDP::sdpMgr.sendSDPPackets(updateSis, SDPFLAG_INCREMENT, dev);
}

void Router::routerWorkingLoop() {
  while (true) {
    std::this_thread::sleep_for(
        std::chrono::seconds(ROUTE_LOOP_INTERVAL + std::rand() % 10));
    LOG_INFO("Sending a routing table to neibours")
    sendRoutingTable(0, {}, {});

    // update metric
    SDP::SDPItemVector updateSis;
    for (auto iter = table.begin(); iter != table.end();) {
      // delete a dead item
      if (iter->metric == SDP_METRIC_DIE) {
        iter = table.erase(iter);
      }
      // ready to delete a item with large matric
      else if (iter->metric >= SDP_METRIC_TIMEOUT) {
        iter->metric = SDP_METRIC_DIE;
        updateSis.push_back(
            SDP::SDPItem(iter->ipPrefix, iter->subNetMask, iter->dist, true));
        ++iter;
      }
      // won't change
      else if (iter->metric == SDP_METRIC_NODEL) {
        ++iter;
      } else {
        iter->metric += 1;
        ++iter;
      }
    }
    Printer::printRouteTable();
    SDP::sdpMgr.sendSDPPackets(updateSis, 0);
  }
}

}  // namespace Route

namespace Printer {
void printRouteItem(const Route::RouteItem& r) {
  printf("  %s/%d  \t%s\t%s\t%s(%d)>%d\n", inet_ntoa(r.ipPrefix),
         Route::maskToPflen(r.subNetMask),
         MAC::toString(r.nextHopMac.addr).c_str(), r.dev->getName().c_str(),
         (r.isDev ? "dev" : "via"), r.dist, r.metric);
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