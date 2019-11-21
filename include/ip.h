/**
 * @file ip.h
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-10-08
 *
 * @brief Library supporting sending/receiving IP packets encapsulated in an
 * Ethernet II frame.
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

  IpPacket() { setDefaultHdr(); };
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
};

/**
 * @brief Send an IP packet
 *
 * @param src src IP
 * @param dest dst IP
 * @param proto protocol. such as TCP
 * @param buf buffer
 * @param len length
 * @return int result
 */
int sendIPPacket(const ip_addr src, const ip_addr dest, int proto,
                 const void* buf, int len);

/**
 * @brief Get the Checksum of a buffer. Will be used in TCP as well
 *
 * @param vdata buffer point
 * @param length length
 * @return uint16_t checksum result
 */
uint16_t getChecksum(const void* vdata, size_t length);

/**
 * @brief Common IP packet callback.
 *
 * It is necessary to check or forward a packet in this function. Then it will
 * submit to `callback` below
 *
 * @param buf buffer
 * @param len length
 * @param id the device getting the packet
 * @return int result
 */
int ipCallBack(const void* buf, int len, DeviceId id);

/**
 * @brief The specific callback for IP packet
 *
 */
extern IPPacketReceiveCallback callback;

/**
 * @brief Get a string from an `in_addr` ip address
 *
 * @param ip ip address
 * @return std::string ip string
 */
std::string ipToStr(const ip_addr& ip);

/**
 * @brief Get a string from an `in_addr` ip address
 *
 * @param ip ip address
 * @param ipstr ip string stored to
 */
void ipToStr(const ip_addr& ip, char* ipstr);

/**
 * @brief Copy an ip address
 *
 * @param dst dst ip address
 * @param src src ip address
 */
void ipCopy(ip_addr& dst, const ip_addr& src);
}  // namespace Ip

namespace Printer {
/**
 * @brief Print IP
 *
 * @param ip ip address
 * @param newline whether a newline after printing
 */
void printIp(const ip_addr& ip, bool newline = true);

/**
 * @brief Print an ip packet
 *
 * @param ipp ip packet
 * @param sender whether it's the sender
 */
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
