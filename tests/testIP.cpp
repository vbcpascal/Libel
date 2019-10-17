#include "api.h"
#include "ip.h"

int myIpCallback(const void* buf, int len) {
  Ip::IpPacket ipp((u_char*)buf, len);
  Printer::printIpPacket(ipp);
  Printer::print((u_char*)buf, len, 14);
  return 0;
}

int main(int argc, char* argv[]) {
  api::init();
  api::setIPPacketReceiveCallback(myIpCallback);
  Device::deviceMgr.addAllDevice();

  std::string srcStr, dstStr, msg;
  std::cout << "Input an ip address to send packet: ";
  std::getline(std::cin, srcStr);
  std::cout << "Input destination ip address: ";
  std::getline(std::cin, dstStr);
  std::cout << "Input your message";
  std::getline(std::cin, msg);

  ip_addr src, dst;
  inet_aton(srcStr.c_str(), &src);
  inet_aton(dstStr.c_str(), &dst);
  api::sendIPPacket(src, dst, 17, msg.c_str(), msg.length());

  Device::deviceMgr.keepReceiving();
  return 0;
}