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
  ether_header_t header;

  /**
   * @brief Length of the frame
   *
   */
  int len;

  EtherFrame() { std::memset(frame, 0, ETHER_MAX_LEN); }

  EtherFrame(const void* buf, int l) : len(l) {
    std::memcpy(frame, buf, len);
    std::memcpy(&header, buf, ETHER_HDR_LEN);
  }

  /**
   * @brief Set the Header object
   *
   * @param hdr the header
   */
  void setHeader(ether_header_t hdr) {
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

  void printFrame(bool brief = true, int col = 2) {
    printf("\033[33m[ PACKET ]\033[0m \t");
    printMAC(header.ether_shost, false);
    printf(" > ");
    printMAC(header.ether_dhost, false);
    printf("\ttype: 0x%04x", header.ether_type);
    printf("\tlen: %d\n", len);
    if (brief) return;

    for (int i = 0; i < len; ++i) {
      if (i != 0 && i % (8 * col) == 0)
        printf("\n");
      else if (i != 0 && i % 8 == 0)
        printf("\t");

      printf("%02x ", frame[i]);
    }
    printf("\n");
  }

  /**
   * @brief A tool funtion to print MAC address
   *
   * @param mac MAC to print
   * @param newline
   */
  static void printMAC(u_char* mac, bool newline = true) {
    for (int i = 0; i < ETHER_ADDR_LEN; ++i) {
      printf("%02x:", mac[i]);
    }
    if (newline) printf("\n");
  }
};

#endif