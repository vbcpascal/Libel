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
#include <sys/socket.h>

#include "arp.h"
#include "router.h"
#include "type.h"

namespace Ip {

/**
 * @brief IGNORE in ip header
 *
 */
constexpr int IGNORE = 0;

/**
 * @brief An IP packet
 *
 */
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

  /**
   * @brief Check Checksum. Use this before ntoh
   *
   * @return int 1 on right. 0 on error. -1 on SDP
   */
  int chkChksum();

  /**
   * @brief hton for ip packet
   *
   */
  void htonType();

  /**
   * @brief ntoh for ip packet
   *
   */
  void ntohType();

  /**
   * @brief Totol length (Reference)
   *
   * @return u_short& length
   */
  u_short& totalLen() { return hdr.ip_len; }

  /**
   * @brief Src ip address (Reference)
   *
   * @return ip_addr& ip
   */
  ip_addr& ipSrc() { return hdr.ip_src; }

  /**
   * @brief Dst ip address (Reference)
   *
   * @return ip_addr& ip
   */
  ip_addr& ipDst() { return hdr.ip_dst; }

  /**
   * @brief Protocol of ip packet (Reference)
   *
   * @return u_char& protocol
   */
  u_char& proto() { return hdr.ip_p; }
};

int sendIPPacket(const ip_addr src, const ip_addr dest, int proto,
                 const void* buf, int len);

uint16_t getChecksum(const void* vdata, size_t length);
int ipCallBack(const void* buf, int len, DeviceId id);
extern IPPacketReceiveCallback callback;

std::string ipToStr(const ip_addr& ip);
void ipToStr(const ip_addr& ip, char* ipstr);
void ipCopy(ip_addr& dst, const ip_addr& src);
}  // namespace Ip

namespace Printer {
void printIp(const ip_addr& ip, bool newline = true);
void printIpPacket(const Ip::IpPacket& ipp, bool sender = false);
}  // namespace Printer

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
