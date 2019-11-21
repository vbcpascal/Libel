/**
 * @file tcpbuffer.h
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-11-15
 *
 * @brief Buffer for TCP. Data received will be stored in `BufferQueue`.
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
  std::queue<size_t> psh_index;
  std::shared_mutex buf_m;
  size_t begin_index = 0;
  size_t end_index = 0;

 public:
  ssize_t write(const u_char* buf, size_t nbyte, bool pshflag = false) {
    std::lock_guard lck(buf_m);
    std::copy_n(buf, nbyte, std::back_inserter(buffer));
    end_index += nbyte;
    if (pshflag) psh_index.push(end_index);
    return nbyte;
  }

  ssize_t read(u_char* buf, size_t nbyte) {
    auto s = can_get(nbyte);
    std::lock_guard lck(buf_m);
    if (s < 0) return -1;
    begin_index += s;
    std::copy_n(buffer.begin(), s, buf);
    for (size_t i = 0; i < nbyte; ++i) buffer.pop_front();
    if (!psh_index.empty() && begin_index > psh_index.front()) psh_index.pop();
    return 0;
  }

  size_t size() {
    std::shared_lock lck(buf_m);
    return buffer.size();
  }

  ssize_t can_get(size_t nbyte) {
    std::shared_lock lck(buf_m);
    if (!psh_index.empty() && begin_index + nbyte >= psh_index.front()) {
      return psh_index.front() - begin_index;
    } else if (buffer.size() >= nbyte) {
      return nbyte;
    } else {
      return -1;
    }
  }
};

#endif
