#include "api.h"

#include <unordered_map>
#include <variant>

namespace api {

std::unordered_map<u_short, commonReceiveCallback> callbackMap;

int callbackDispatcher(const void* buf, int len, DeviceId id) {
  auto frame = Ether::EtherFrame(buf, len);
  if (frame.getLength() == 0) return 0;

  // Printer::printEtherFrame(frame);

  frame.ntohType();
  auto hdr = frame.getHeader();
  auto dev = Device::deviceMgr.getDevicePtr(id);

  // src is me?
  if (MAC::isSameMacAddr(dev->getMAC(), hdr.ether_shost)) {
    // ignore it;
    return 0;
  }

  // dst is me or broadcast?
  if (MAC::isSameMacAddr(dev->getMAC(), hdr.ether_dhost) ||
      MAC::isBroadcast(dev->getMAC())) {
    u_short type = hdr.ether_type;

    auto iter = callbackMap.find(type);

    if (iter == callbackMap.end()) {
      LOG_ERR("Callback function not found");
      return -1;
    } else {
      if (iter->second)
        return iter->second(frame.getPayload(), len - ETHER_HDR_LEN, id);
      else
        return -1;
    }
  }

  return 0;
}

int setCallback(u_short etherType, commonReceiveCallback callback) {
  return (callbackMap.insert_or_assign(etherType, callback)).second;
}

int init() {
  setFrameReceiveCallback(callbackDispatcher);
  setCallback(ETHERTYPE_ARP, Arp::arpCallBack);
  setCallback(ETHERTYPE_IP, Ip::ipCallBack);
  return 0;
}

int sendFrame(const void* buf, int len, int ethtype, const void* destmac,
              DeviceId id) {
  return Device::deviceMgr.sendFrame(buf, len, ethtype, destmac, id);
}

int setFrameReceiveCallback(frameReceiveCallback callback) {
  LOG_INFO("Set new callback function.");
  Device::callback = callback;
  return 0;
}

int sendIPPacket(const struct in_addr src, const struct in_addr dest, int proto,
                 const void* buf, int len) {
  return Ip::sendIPPacket(src, dest, proto, buf, len);
}

int setIPPacketReceiveCallback(IPPacketReceiveCallback callback) {
  Ip::callback = callback;
  return 0;
}

int setRoutingTable(const in_addr dest, const in_addr mask,
                    const void* nextHopMAC, const char* device) {
  auto nm = MAC::macAddr((const u_char*)nextHopMAC);
  auto dev = Device::deviceMgr.getDevicePtr(std::string(device));
  return Route::router.setTable(dest, mask, nm, dev);
}

int addAllDevice(bool sniff) { return Device::deviceMgr.addAllDevice(sniff); }

}  // namespace api