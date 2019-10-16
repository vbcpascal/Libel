#ifndef ARP_H_
#define ARP_H_

#include <arpa/inet.h>
#include <netinet/ip.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

#include "device.h"
#include "ether.h"
#include "massert.h"
#include "type.h"

namespace Arp {

constexpr int ARP_TIMEOUT = 1;

/**
 * @brief the value of ar_op defined in if_arp.h
 *
 * should be ARPOP_REQUEST or ARPOP_REPLY in this lab;
 *
 */
using arpOpType = int;

class ArpFrame {
 public:
  struct __attribute__((__packed__)) {
    arphdr arpHdr;
    u_char srcMac[6];
    ip_addr srcIp;
    u_char dstMac[6];
    ip_addr dstIp;
  };

  ArpFrame() = default;
  ArpFrame(const void* buf) {
    memcpy(&(this->arpHdr.ar_hrd), buf, sizeof(ArpFrame));
  }
  void setDefaultHdr(arpOpType opType = ARPOP_REQUEST);

  void htonType();
  void ntohType();
};

class ArpManager {
 public:
  std::map<ip_addr, MAC::macAddr> ipMacMap;
  bool found;
  std::condition_variable cv;
  std::mutex cv_m;  // mutex for cv

  MAC::macAddr getMacAddr(Device::DevicePtr dev, const ip_addr& dstIp,
                          int maxRetry = 5);
  int sendRequestArp(Device::DevicePtr dev, const ip_addr& dstIp, int maxRetry);
  void sendReplyArp(Device::DevicePtr, const u_char* dstMac,
                    const ip_addr& dstIp);
};

int arpCallBack(const void* buf, int len, DeviceId id);
extern ArpManager arpMgr;
}  // namespace Arp

namespace Printer {
void printArpFrame(const Arp::ArpFrame& af);
}
#endif