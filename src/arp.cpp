#include "arp.h"

namespace Arp {

ArpManager arpMgr;

int arpCallBack(const void* buf, int len, DeviceId id) {
  auto dev = Device::deviceMgr.getDevicePtr(id);
  ArpFrame frame(buf);

  // if (MAC::isSameMacAddr(dev->getMAC(), frame.srcMac)) return 0;

  frame.ntohType();
  Printer::printArpFrame(frame);
  switch (frame.arpHdr.ar_op) {
    case ARPOP_REPLY: {
      arpMgr.ipMacMap[frame.srcIp] = MAC::macAddr(frame.dstMac);
      break;
    }
    case ARPOP_REQUEST: {
      arpMgr.sendReplyArp(dev, frame.srcMac, frame.srcIp);
      break;
    }
    default: {
      LOG_ERR("Unsupported arp op");
      break;
    }
  }
  return 0;
}

void ArpFrame::setDefaultHdr(arpOpType opType) {
  // http://www.tcpipguide.com/free/t_ARPMessageFormat.htm
  arpHdr.ar_hrd = ARPHRD_ETHER;    // format of hardware address
  arpHdr.ar_pro = ETHERTYPE_IP;    // format of protocol address
  arpHdr.ar_hln = ETHER_ADDR_LEN;  // length of hardware address
  arpHdr.ar_pln = 4;               // length of protocol address
  arpHdr.ar_op = opType;
}

void ArpFrame::htonType() {
  arpHdr.ar_hrd = htons(arpHdr.ar_hrd);
  arpHdr.ar_pro = htons(arpHdr.ar_pro);
  arpHdr.ar_op = htons(arpHdr.ar_op);
}

void ArpFrame::ntohType() {
  arpHdr.ar_hrd = ntohs(arpHdr.ar_hrd);
  arpHdr.ar_pro = ntohs(arpHdr.ar_pro);
  arpHdr.ar_op = ntohs(arpHdr.ar_op);
}

MAC::macAddr ArpManager::getMacAddr(Device::DevicePtr dev, const ip_addr& dstIp,
                                    int maxRetry) {
  auto iter = ipMacMap.find(dstIp);
  if (iter != ipMacMap.end()) return iter->second;

  LOG_INFO("Send ARP request to %s", inet_ntoa(dstIp));
  int res = sendRequestArp(dev, dstIp, maxRetry);
  if (res >= 0) {
    iter = ipMacMap.find(dstIp);
    LOG_INFO("Get mac address: %s", MAC::toString(iter->second.addr).c_str());
    return iter->second;
  } else {
    LOG_WARN("MAC not found.");
    return MAC::macAddr();
  }
}

int ArpManager::sendRequestArp(Device::DevicePtr dev, const ip_addr& dstIp,
                               int maxRetry) {
  std::unique_lock<std::mutex> lock(cv_m);
  // lock.lock();

  ArpFrame frame;
  frame.setDefaultHdr(ARPOP_REQUEST);
  frame.srcIp = dev->getIp();
  frame.dstIp = dstIp;
  std::memcpy(frame.srcMac, dev->getMAC(), ETHER_ADDR_LEN);
  std::memcpy(frame.dstMac, Ether::zeroMacAddr, ETHER_ADDR_LEN);
  frame.htonType();
  Device::deviceMgr.sendFrame(&frame, sizeof(ArpFrame), ETHERTYPE_ARP,
                              Ether::broadcastMacAddr, dev);

  // wait ...
  int cnt = 0;
  int found = -1;
  for (int i = maxRetry + 1; i != 0; --i, ++cnt) {
    if (cv.wait_for(lock, std::chrono::seconds(ARP_TIMEOUT), [&] {
          auto iter = ipMacMap.find(dstIp);
          return iter != ipMacMap.end();
        })) {
      // LOG_INFO("wait_for finished.");
      auto iter = ipMacMap.find(dstIp);
      if (iter != ipMacMap.end()) {
        found = 0;
        break;
      }
    } else {
      LOG_WARN("Request timeout for arp seq %d, dstip = %s", cnt,
               inet_ntoa(dstIp));
    }
  }

  lock.unlock();
  return found;
}

void ArpManager::sendReplyArp(Device::DevicePtr dev, const u_char* dstMac,
                              const ip_addr& dstIp) {
  LOG_INFO("Send ARP reply to %s", inet_ntoa(dstIp));
  ArpFrame frame;
  frame.setDefaultHdr(ARPOP_REPLY);
  frame.srcIp = dev->getIp();
  frame.dstIp = dstIp;
  std::memcpy(frame.srcMac, dev->getMAC(), ETHER_ADDR_LEN);
  std::memcpy(frame.dstMac, dstMac, ETHER_ADDR_LEN);
  frame.htonType();
  Device::deviceMgr.sendFrame(&frame, sizeof(ArpFrame), ETHERTYPE_ARP, dstMac,
                              dev);
}
}  // namespace Arp

namespace Printer {

std::map<u_short, std::string> arpOpTypeNameMap{
    {ARPOP_REQUEST, "\033[35mRequest\033[0m"},
    {ARPOP_REPLY, "\033[36mREPLY\033[0m"}};

void printArpFrame(const Arp::ArpFrame& af) {
  printf("  ARP %s\t", arpOpTypeNameMap[af.arpHdr.ar_op].c_str());
  printf("- MAC : %s -> %s\t", MAC::toString(af.srcMac).c_str(),
         MAC::toString(af.dstMac).c_str());
  char srcIpStr[20], dstIpStr[20];
  strcpy(srcIpStr, inet_ntoa(af.srcIp));
  strcpy(dstIpStr, inet_ntoa(af.dstIp));
  printf("IP : %s -> %s\n", srcIpStr, dstIpStr);
}
}  // namespace Printer