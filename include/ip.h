/**
 * @file ip.h
 * @author guanzhichao
 * @brief Library supporting sending/receiving IP packets encapsulated in an
 * Ethernet II frame.
 * @version 0.1
 * @date 2019-10-08
 *
 */

#ifndef IP_H_
#define IP_H_

#include <netinet/ip.h>

#include "arp.h"
#include "router.h"
#include "type.h"

namespace Ip {

constexpr int IGNORE = 0;

class IpPacket {
 public:
  struct __attribute__((__packed__)) {
    ip hdr;
    u_char data[IP_MAXPACKET];
  };

  IpPacket() = default;
  IpPacket(const u_char* buf, int len);

  void setDefaultHdr();
  int setData(const u_char* buf, int len);

  // use this after hton
  void setChksum();

  // use this before ntoh
  bool chkChksum();

  void hton();
  void ntoh();

  u_short& totalLen() { return hdr.ip_len; }
  ip_addr& ipSrc() { return hdr.ip_src; }
  ip_addr& ipDst() { return hdr.ip_dst; }
  u_char& proto() { return hdr.ip_p; }
};

int sendIPPacket(const ip_addr src, const ip_addr dest, int proto,
                 const void* buf, int len);

uint16_t getChecksum(const void* vdata, size_t length);
int ipCallBack(const void* buf, int len, DeviceId id);
extern IPPacketReceiveCallback callback;
}  // namespace Ip

namespace Printer {
void printIpPacket(const Ip::IpPacket& ipp);
}

/*
 * Internet Datagram Header
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Version|  IHL  |Type of Service|          Total Length         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |         Identification        |Flags|      Fragment Offset    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Time to Live |    Protocol   |         Header Checksum       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       Source Address                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Destination Address                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

#endif  // IP_H_
