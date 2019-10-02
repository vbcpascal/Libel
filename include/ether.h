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
  int len;

  EtherFrame() { std::memset(frame, 0, ETHER_MAX_LEN); }

  void setHeader(ether_header_t hdr) {
    std::memcpy(frame, &hdr, ETHER_HDR_LEN);
  }

  void setPayload(const void* buf, int l) {
    std::memcpy(frame + ETHER_HDR_LEN, buf, l);
    len = l;
  }

  void setChecksum() { ASSERT(false, "not implemented."); }
};

#endif