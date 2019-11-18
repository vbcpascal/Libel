/**
 * @file socketaddr.h
 * @author guanzhichao
 * @brief
 * @version 0.1
 * @date 2019-11-02
 *
 * @copyright Copyright (c) 2019
 *
 */

#ifndef SOCKETADDR_H_
#define SOCKETADDR_H_

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "ip.h"

namespace Socket {
struct SocketAddr {
  ip_addr ip;
  int port;

  SocketAddr() : port(0) { ip.s_addr = 0; }
  SocketAddr(const ip_addr& ip, int p) : ip(ip), port(p) {}
  SocketAddr(const sockaddr* addr) { set(addr); }

  bool isZero() { return (ip.s_addr == 0 && port == 0); }

  void set(const sockaddr* addr) {
    auto addr_in = reinterpret_cast<const sockaddr_in*>(addr);
    Ip::ipCopy(ip, addr_in->sin_addr);
    port = addr_in->sin_port;
  }

  void get(sockaddr* addr) {
    auto addr_in = reinterpret_cast<sockaddr_in*>(addr);
    Ip::ipCopy(addr_in->sin_addr, ip);
    addr_in->sin_family = AF_INET;
    addr_in->sin_port = port;
    addr_in->sin_len = INET_ADDRSTRLEN;
  }

  bool operator==(const SocketAddr& sa) {
    return ip.s_addr == sa.ip.s_addr && port == sa.port;
  }
};
}  // namespace Socket
#endif
