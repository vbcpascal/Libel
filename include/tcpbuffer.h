/**
 * @file tcpbuffer.h
 * @author guanzhichao
 * @brief
 * @version 0.1
 * @date 2019-11-18
 *
 */

#ifndef TCPBUFFER_H_
#define TCPBUFFER_H_

#include <deque>

#include "massert.h"
#include "type.h"

class BufferQueue {
 private:
  std::deque<u_char> buffer;

 public:
  int write(const u_char* buf, size_t nbyte) {
    std::copy_n(buf, nbyte, std::back_inserter(buffer));
    return 0;
  }

  int read(u_char* buf, size_t nbyte) {
    if (buffer.size() < nbyte) return -1;
    std::copy_n(buffer.begin(), nbyte, buf);
    for (size_t i = 0; i < nbyte; ++i) buffer.pop_front();
    return 0;
  }

  int size() { return buffer.size(); }
};

#endif
