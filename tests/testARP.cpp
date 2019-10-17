#include "api.h"
#include "arp.h"
#include "device.h"

int main(int argc, char* argv[]) {
  char devName[20], addr[25];
  scanf("please input a device name: %s\n", devName);
  scanf("please input an ip address: %s\n", addr);

  api::init();
  DeviceId id;
  id = addDevice(devName);
  if (id < 0) {
    LOG_ERR("Device not found: %s", argv[0]);
    return 0;
  }
  Device::DevicePtr dev = Device::deviceMgr.getDevicePtr(id);

  ip_addr ip;
  int res = inet_aton(addr, &ip);
  if (res <= 0) {
    LOG_ERR("Ip address error: %s", argv[2]);
    return 0;
  }
  Arp::arpMgr.getMacAddr(dev, ip, 5);
  Printer::printMAC(Arp::arpMgr.ipMacMap[ip].addr);

  Device::deviceMgr.keepReceiving();
  return 0;
}