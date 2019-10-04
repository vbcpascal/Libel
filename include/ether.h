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

#define e_PRINT_NONE 0
#define e_PRINT_INTRO 1
#define e_PRINT_MAC 2
#define e_PRINT_TYPE 4
#define e_PRINT_LEN 8
#define e_PRINT_ALL 127

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
  /**
   * @brief A Ethernet frame
   *
   */
  u_char frame[ETHER_MAX_LEN];

  /**
   * @brief Header of the frame
   *
   * We cannot promise that it will match the header in frame
   *
   */
  ether_header header;

  /**
   * @brief Length of the frame
   *
   */
  int len;

  EtherFrame();

  /**
   * @brief Construct a new Ether Frame object
   *
   * @param buf frame buffer
   * @param l length of the buffer
   */
  EtherFrame(const void* buf, int l);

  u_char* getPayload() { return frame + ETHER_HDR_LEN; }

  int getPayloadLength() { return len - ETHER_HDR_LEN - ETHER_CRC_LEN; }

  /**
   * @brief Set the Header object
   *
   * @param hdr the header
   */
  void setHeader(ether_header hdr) {
    header = hdr;
    updateHeader();
  }

  /**
   * @brief Update header. Always transfer this function when you change header
   * directly.
   *
   */
  void updateHeader() {
    std::memcpy(frame, &header, ETHER_HDR_LEN);
    return;
  }

  /**
   * @brief Set the Payload object
   *
   * @param buf the payload buffer
   * @param l length of payload
   */
  void setPayload(const void* buf, int l) {
    std::memcpy(frame + ETHER_HDR_LEN, buf, l);
    len = l + ETHER_HDR_LEN + ETHER_CRC_LEN;
  }

  /**
   * @brief Set the Checksum object (Not implemented)
   *
   */
  void setChecksum() { ASSERT(false, "not implemented."); }

  /**
   * @brief Print the frame
   *
   * @param col show brief information if col is 0
   */
  void printFrame(int col = 0, int option = e_PRINT_ALL);
};

#endif
