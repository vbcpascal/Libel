#include "packetio.h"

extern Device::DeviceManager Device::deviceMgr;
extern frameReceiveCallback Device::callback;

int sendFrame(const void* buf, int len, int ethtype, const void* destmac,
              DeviceId id) {
  return Device::deviceMgr.sendFrame(buf, len, ethtype, destmac, id);
}

int setFrameReceiveCallback(frameReceiveCallback callback) {
  LOG_INFO("Set new callback function.");
  Device::callback = callback;
  return 0;
}