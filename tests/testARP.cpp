#include "api.h"
#include "arp.h"
#include "device.h"

int main(int argc, char* argv[]) {
  std::string devName, addr;
  printf("please input a device name: ");
  std::cin >> devName;
  printf("please input an ip address: ");
  std::cin >> addr;

  api::init();
  DeviceId id;
  id = addDevice(devName.c_str());
  if (id < 0) {
    LOG_ERR("Device not found: %s", argv[0]);
    return 0;
  }
  Device::DevicePtr dev = Device::deviceMgr.getDevicePtr(id);

  ip_addr ip;
  int res = inet_aton(addr.c_str(), &ip);
  if (res <= 0) {
    LOG_ERR("Ip address error: %s", argv[2]);
    return 0;
  }
  Arp::arpMgr.getMacAddr(dev, ip, 5);
  Printer::printMAC(Arp::arpMgr.ipMacMap[ip].addr);

  Device::deviceMgr.keepReceiving();
  return 0;
}