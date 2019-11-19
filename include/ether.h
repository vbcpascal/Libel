/**
 * @file ether.h
 * @author guanzhichao
 * @brief Library supporting a EtherFrame class.
 * @version 0.1
 * @date 2019-10-02
 *
 */

#ifndef ETHER_H_
#define ETHER_H_

#include <netinet/ip.h>

#include <cstdint>
#include <cstring>

#include "type.h"

#ifdef __APPLE__
#ifndef ETH_ZLEN
#define ETH_ZLEN 60
#endif
#endif

// print frame option
constexpr int e_PRINT_NONE = 0;
constexpr int e_PRINT_INTRO = 1;
constexpr int e_PRINT_MAC = 2;
constexpr int e_PRINT_TYPE = 4;
constexpr int e_PRINT_LEN = 8;
constexpr int e_PRINT_ALL = 127;

namespace MAC {

/**
 * @brief A class storing a MAC address
 *
 */
class MacAddr {
 public:
  u_char addr[6];

  MacAddr() {
    for (int i = 0; i < 6; ++i) addr[i] = 255;
  }
  MacAddr(const u_char* _mac) { memcpy(addr, _mac, ETHER_ADDR_LEN); }

  const MacAddr& operator=(const MacAddr& m);
};

bool operator==(const MacAddr& ml, const MacAddr& mr);
bool isSameMacAddr(const u_char* macA, const u_char* macB);

/**
 * @brief Whether a mac address is broadcast
 *
 * @param mac mac address
 * @return true is broadcast
 * @return false is not broadcast
 */
bool isBroadcast(const u_char* mac);

/**
 * @brief Whether a mac address is broadcast
 *
 * @param mac mac address
 * @return true is broadcast
 * @return false is not broadcast
 */
bool isBroadcast(const MacAddr& mac);

/**
 * @brief Convert a MAC address to cpp string
 *
 * @param mac MAC address
 * @return std::string MAC address string
 */
std::string toString(const u_char* mac);

/**
 * @brief Convert a string to MAC address
 *
 * @param mac dst MAC address
 * @param str MAC address string
 */
void strtoMAC(u_char* mac, const char* str);

}  // namespace MAC

namespace Ether {

extern const u_char broadcastMacAddr[6];
extern const u_char zeroMacAddr[6];

/**
 * @brief Store a Ethernet frame
 *
 */
class EtherFrame {
 public:
  struct __attribute__((__packed__)) {
    ether_header header;
    u_char payload[ETHER_MAX_LEN];
#ifdef ETHER_CRC_OPEN
    u_char crc[ETHER_CRC_LEN];
#endif
  } frame;

  int len;

  EtherFrame();

  /**
   * @brief Construct a new Ether Frame object
   *
   * @param buf frame buffer
   * @param l length of the buffer
   */
  EtherFrame(const void* buf, int l);

  /**
   * @brief Get the Frame object
   *
   * @return u_char* frame
   */
  u_char* getFrame() { return (u_char*)&frame; }

  /**
   * @brief Get the Payload object
   *
   * @return u_char* payload pointer
   */
  u_char* getPayload() { return frame.payload; }

  /**
   * @brief Get the Header object
   *
   * @return ether_header header
   */
  ether_header getHeader() { return frame.header; }

  /**
   * @brief Get the Length object
   *
   * @return int length
   */
  int getLength() { return len; }

  /**
   * @brief Get the Payload Length object
   *
   * @return int length
   */
#ifdef ETHER_CRC_OPEN
  int getPayloadLength() { return len - ETHER_HDR_LEN - ETHER_CRC_LEN; }
#else
  int getPayloadLength() { return len - ETHER_HDR_LEN; }
#endif

  /**
   * @brief Set the Header object
   *
   * @param hdr the header
   */
  void setHeader(ether_header hdr) { frame.header = hdr; }

  /**
   * @brief Set the Payload object
   *
   * @param buf the payload buffer
   * @param l length of payload
   */
  void setPayload(const u_char* buf, int l) {
    std::memcpy(frame.payload, buf, l);
#ifdef ETHER_CRC_OPEN
    len = l + ETHER_HDR_LEN + ETHER_CRC_LEN;
#else
    len = l + ETHER_HDR_LEN;
#endif
  }

  /**
   * @brief Padding. You should always transfer this before you send a ether
   * frame
   *
   */
  void padding() {
    if (len < ETHER_MIN_LEN - 4) len = ETHER_MIN_LEN - 4;
  }

  /**
   * @brief ntoh for Ether frame
   *
   */
  void ntohType() { frame.header.ether_type = ntohs(frame.header.ether_type); }

  /**
   * @brief hton for Ether frame
   *
   */
  void htonType() { frame.header.ether_type = htons(frame.header.ether_type); }
};
}  // namespace Ether

namespace Printer {

/**
 * @brief Print a MAC address
 *
 * @param mac MAC address
 * @param end string print at the end
 */
void printMAC(const u_char* mac, const std::string end = "\n");

/**
 * @brief Print a EtherFrame object
 *
 * @param ef EtherFrame object
 * @param col column shown for frame payload. if col = 0, only brief information
 * will be shown.
 * @param option option
 */
void printEtherFrame(const Ether::EtherFrame& ef, int col = 0,
                     int option = e_PRINT_ALL);

/**
 * @brief print a common payload data
 *
 * @param buf buffer to print
 * @param len length to print
 * @param placeholder number of placeholder
 */
void print(const u_char* buf, int len, int placeholder = 0);
}  // namespace Printer

#endif  // ETHER_H_
