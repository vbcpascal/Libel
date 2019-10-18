/**
 * @file arp.h
 * @author guanzhichao
 * @brief Library supporting sending and receiving ARP frame.
 * @version 0.1
 * @date 2019-10-18
 *
 */

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

/**
 * @brief timeout of arp reply (second)
 *
 */
constexpr int ARP_TIMEOUT = 1;

/**
 * @brief The value of ar_op defined in if_arp.h
 *
 * should be ARPOP_REQUEST or ARPOP_REPLY in this lab;
 *
 */
using arpOpType = int;

/**
 * @brief An ARP frame
 *
 */
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

/**
 * @brief Manage all ARP items, request/reply an ARP frame
 *
 */
class ArpManager {
 public:
  std::map<ip_addr, MAC::MacAddr> ipMacMap;
  std::condition_variable cv;
  std::mutex cv_m;  // mutex for cv

  MAC::MacAddr getMacAddr(Device::DevicePtr dev, const ip_addr& dstIp,
                          int maxRetry = 5);
  int sendRequestArp(Device::DevicePtr dev, const ip_addr& dstIp, int maxRetry);
  void sendReplyArp(Device::DevicePtr, const u_char* dstMac,
                    const ip_addr& dstIp);
};

/**
 * @brief Default ARP frame callback function.
 *
 * @param buf pointer to the payload of Ethernet frame received
 * @param len the length of payload of Ethernet frame received
 * @param id id of device
 * @return int 0 on success, -1 on error
 */
int arpCallBack(const void* buf, int len, DeviceId id);

/**
 * @brief unique ARP Manager
 *
 */
extern ArpManager arpMgr;
}  // namespace Arp

namespace Printer {

/**
 * @brief Print current ARP table
 *
 */
void printArpTable();

/**
 * @brief Print an ARP frame
 *
 * @param af ARP frame
 * @param sender is sender or receiver
 */
void printArpFrame(const Arp::ArpFrame& af, bool sender = false);
}  // namespace Printer
#endif