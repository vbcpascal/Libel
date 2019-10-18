#include "api.h"
#include "arp.h"
#include "device.h"

int main(int argc, char* argv[]) {
  printf("Usage: ./testARP en0 1.2.3.4\n");
  char* devName = argv[argc - 2];
  char* addr = argv[argc - 1];
  printf(
      "Try to get MAC address of \033[33;1m%s\033[0m, via "
      "\033[33;1m%s\033[0m\n",
      addr, devName);

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
  printf("MAC address: ");
  Printer::printMAC(Arp::arpMgr.ipMacMap[ip].addr);
  Printer::printArpTable();

  Device::deviceMgr.keepReceiving();
  return 0;
}