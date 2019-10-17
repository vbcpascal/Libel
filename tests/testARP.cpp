#include "api.h"
#include "arp.h"
#include "device.h"

int main(int argc, char* argv[]) {
  if (argc <= 2) {
    printf("please input a device name and an ip address.\n");
    printf("example: ./testARP en0 1.2.3.4\n");
  }

  api::init();
  DeviceId id;
  if (argc <= 1)
    id = addDevice("en0");
  else
    id = addDevice(argv[1]);
  if (id < 0) {
    LOG_ERR("Device not found: %s", argv[0]);
    return 0;
  }
  Device::DevicePtr dev = Device::deviceMgr.getDevicePtr(id);

  ip_addr ip;
  int res;
  if (argc <= 2)
    res = inet_aton("1.2.3.4", &ip);
  else
    res = inet_aton(argv[2], &ip);

  if (res <= 0) {
    LOG_ERR("Ip address error: %s", argv[2]);
    return 0;
  }
  Arp::arpMgr.getMacAddr(dev, ip, 5);
  Printer::printMAC(Arp::arpMgr.ipMacMap[ip].addr);

  Device::deviceMgr.keepReceiving();
  return 0;
}