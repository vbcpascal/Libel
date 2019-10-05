/**
 * @file ether.h
 * @author guanzhichao
 * @brief File supporting a EtherFrame class.
 * @version 0.1
 * @date 2019-10-02
 *
 */

#ifndef ETHER_H
#define ETHER_H

#include <cstdint>
#include <cstring>

#include "type.h"

#ifndef ETH_ZLEN
#define ETH_ZLEN 60
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
 * @brief Convert a MAC address to cpp string
 *
 * @param mac MAC address
 * @return std::string MAC address string
 */
std::string toString(const u_char* mac);

/**
 * @brief A tool funtion to print MAC address
 *
 * @param mac MAC to print
 * @param newline
 */
void printMAC(const u_char* mac, bool newline = true);

/**
 * @brief Convert a string to MAC address
 *
 * @param mac dst MAC address
 * @param str MAC address string
 */
void strtoMAC(u_char* mac, const char* str);

}  // namespace MAC

/**
 * @brief Store a Ethernet frame
 *
 */
class EtherFrame {
 public:
  struct __attribute__((__packed__)) {
    ether_header header;
    u_char payload[ETHER_MAX_LEN];
    u_char crc[ETHER_CRC_LEN];
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
  int getPayloadLength() { return len - ETHER_HDR_LEN - ETHER_CRC_LEN; }

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
    len = l + ETHER_HDR_LEN + ETHER_CRC_LEN;
  }

  /**
   * @brief Set the Checksum object (Not implemented)
   *
   */
  void setCRC(const u_char* crcBuf) {
    std::memcpy(frame.crc, crcBuf, ETHER_CRC_LEN);
  }

  /**
   * @brief Print the frame
   *
   * @param col show brief information if col is 0
   */
  void printFrame(int col = 0, int option = e_PRINT_ALL);

  void ntohType() { frame.header.ether_type = ntohs(frame.header.ether_type); }

  void htonType() { frame.header.ether_type = htons(frame.header.ether_type); }
};

#endif
