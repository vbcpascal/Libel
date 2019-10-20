#include "api.h"
#include "router.h"
#include "sdp.h"

int main(int argc, char* argv[]) {
  api::init();
  api::addAllDevice(true);
  api::initRouter();

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