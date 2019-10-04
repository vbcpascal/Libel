#include "packetio.h"

extern Device::DeviceController Device::deviceCtrl;
extern frameReceiveCallback Device::callback;

int sendFrame(const void* buf, int len, int ethtype, const void* destmac,
              int id) {
  if (!ETHER_IS_VALID_LEN(len + ETHER_HDR_LEN)) {
    LOG(ERR, "len is too large: %d.", len);
    return -1;
  }

  ether_header hdr;
  hdr.ether_type = (u_short)ethtype;
  std::memcpy(hdr.ether_dhost, destmac, ETHER_ADDR_LEN);

  EtherFrame frame;
  frame.setHeader(hdr);
  frame.setPayload(buf, len);
  return Device::deviceCtrl.sendFrame(id, frame);
}

int setFrameReceiveCallback(frameReceiveCallback callback) {
  LOG(INFO, "Set new callback function.");
  Device::callback = callback;
  return 0;
}