/**
 * @file testDeviceManager.cpp
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-10-02
 *
 * @brief Test(1): open all devices and begin listening on them. An warning will
 * be shown when trying to add a device opened.
 *
 */

#include "api.h"
#include "device.h"
#include "ether.h"
#include "type.h"

// It's useless unless you want to print a frame...
int myCallback(const void* buf, int len, DeviceId id) {
  Ether::EtherFrame frame;
  frame.setPayload((u_char*)buf, len);
  LOG(" CALLBACK FUNTION ", "frame printed below");
  Printer::printEtherFrame(frame, 2,
                           e_PRINT_INTRO + e_PRINT_TYPE + e_PRINT_LEN);
  return 0;
}

int main(int argc, char* argv[]) {
  // choose one between next two
  api::setFrameReceiveCallback(myCallback);

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
  DeviceId id = findDevice(devName.c_str());
  LOG("RES ", "find device. id: %d: ", id);

  // keep receiving packets from all devices
  keepReceiving();
  return 0;
}
