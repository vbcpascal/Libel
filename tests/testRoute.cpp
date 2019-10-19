#include "api.h"
#include "router.h"

int myIpCallback(const void* buf, int len) {
  Ip::IpPacket ipp((u_char*)buf, len);
  Printer::printIpPacket(ipp);
  return 0;
}

int main(int argc, char* argv[]) {
  api::init();
  api::setIPPacketReceiveCallback(myIpCallback);
  api::addAllDevice(true);

  std::cout << "Set Routing table. input \"end\" to break.\n  [devname] "
               "[destip] [mask] [mac]\n";
  std::string devName, destStr, maskStr, macStr, cmd;
  ip_addr destIp, maskIp;
  Device::DevicePtr dev;
  u_char mac[6];

  while (std::cin >> devName) {
    if (devName == "end") break;
    std::cin >> destStr >> maskStr >> macStr;
    if (!(dev = Device::deviceMgr.getDevicePtr(devName.c_str()))) {
      LOG_ERR("No device: %s", devName.c_str());
      continue;
    }
    inet_aton(destStr.c_str(), &destIp);
    inet_aton(maskStr.c_str(), &maskIp);
    MAC::strtoMAC(mac, macStr.c_str());
    api::setRoutingTable(destIp, maskIp, mac, devName.c_str());
    LOG_INFO("Add routing item succeed!")
  }

  Printer::printRouteTable();

  std::string srcStr, dstStr, msg, tmp;
  while (true) {
    std::cout << "Input an ip address to send packet: ";
    std::cin >> tmp;
    if (tmp != " ") srcStr = tmp;
    if (tmp == "end") break;
    std::cout << "Input destination ip address: ";
    std::cin >> tmp;
    if (tmp != " ") dstStr = tmp;
    std::cout << "Input your message: ";
    std::cin >> tmp;
    if (tmp != " ") msg = tmp;
    std::cout << "length of messsage: " << msg.length() << std::endl;
    std::cout << srcStr << " -> " << dstStr << " : " << msg << std::endl;

    ip_addr src, dst;
    inet_aton(srcStr.c_str(), &src);
    inet_aton(dstStr.c_str(), &dst);
    api::sendIPPacket(src, dst, IPPROTO_UDP, msg.c_str(), msg.length() + 1);
  }

  Device::deviceMgr.keepReceiving();
  return 0;
}