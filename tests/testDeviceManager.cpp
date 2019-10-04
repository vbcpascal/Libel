#include "device.h"
#include "packetio.h"
#include "type.h"

int myCallback(const void* buf, int len, int id) {
  EtherFrame frame;
  frame.setPayload(buf, len);
  // LOG(" CALLBACK FUNTION ", "frame printed below");
  // frame.printFrame(2, e_PRINT_INTRO + e_PRINT_TYPE + e_PRINT_LEN);
  return 0;
}

int main() {
  setFrameReceiveCallback(&myCallback);

  // add all device
  int cnt = Device::deviceMgr.addAllDevice();
  LOG_INFO("Add %d devices.", cnt);
  if (cnt == 0) {
    LOG_WARN("No device added. Exit");
    return 0;
  }

  // add an exist device, warning here
  std::string devName = Device::deviceMgr.getDevicePtr(0)->getName().c_str();
  addDevice(devName.c_str());

  // find a device
  int id = findDevice(devName.c_str());
  LOG("RES ", "find device. id: %d: ", id);

  // keep receiving packets from all devices
  keepReceiving();
  return 0;
}
