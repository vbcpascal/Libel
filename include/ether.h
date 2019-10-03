/**
 * @file ether.h
 * @author guanzhichao
 * @brief
 * @version 0.1
 * @date 2019-10-02
 *
 * @copyright Copyright (c) 2019
 *
 */

#ifndef ETHER_H
#define ETHER_H

#include <cstdint>
#include <cstring>

#include "type.h"

class EtherFrame {
 public:
  uint8_t frame[ETHER_MAX_LEN];
  ether_header_t header;
  int len;

  EtherFrame() { std::memset(frame, 0, ETHER_MAX_LEN); }

  void setHeader(ether_header_t hdr) {
    header = hdr;
    updateHeader();
  }

  // always transfer this function when you change header
  void updateHeader() {
    std::memcpy(frame, &header, ETHER_HDR_LEN);
    return;
  }

  void setPayload(const void* buf, int l) {
    std::memcpy(frame + ETHER_HDR_LEN, buf, l);
    len = l + ETHER_HDR_LEN + ETHER_CRC_LEN;
  }

  void setChecksum() { ASSERT(false, "not implemented."); }
};

#endif