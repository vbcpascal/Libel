#include "api.h"

#include <unordered_map>

namespace api {

std::unordered_map<u_short, commonReceiveCallback> callbackMap;

int callbackDispatcher(const void* buf, int len, DeviceId id) {
  auto frame = Ether::EtherFrame(buf, len);
  if (frame.getLength() == 0) return 0;

  frame.ntohType();
  auto hdr = frame.getHeader();

  // is me or broadcast?
  auto dev = Device::deviceMgr.getDevicePtr(id);
  if (MAC::isSameMacAddr(dev->getMAC(), hdr.ether_dhost) ||
      MAC::isBroadcast(dev->getMAC())) {
    u_short type = hdr.ether_type;

    auto iter = callbackMap.find(type);

    if (iter == callbackMap.end()) {
      // LOG_ERR("Callback function not found");
      // return -1;
      return 0;
    } else {
      if (iter->second)
        return iter->second(frame.getPayload(), len, id);
      else
        return -1;
    }
  }
  // TODO route it!
  else {
    return 0;
  }
}

int setCallback(u_short etherType, commonReceiveCallback callback) {
  return (callbackMap.insert_or_assign(etherType, callback)).second;
}

int init() {
  setFrameReceiveCallback(callbackDispatcher);
  setCallback(ETHERTYPE_ARP, Arp::arpCallBack);
  return 0;
}

int setFrameReceiveCallback(frameReceiveCallback callback) {
  LOG_INFO("Set new callback function.");
  Device::callback = callback;
  return 0;
}

int sendFrame(const void* buf, int len, int ethtype, const void* destmac,
              DeviceId id) {
  return Device::deviceMgr.sendFrame(buf, len, ethtype, destmac, id);
}

}  // namespace api