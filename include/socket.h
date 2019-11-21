/**
 * @file socket.h
 * @author guanzhichao
 * @brief POSIX-compatible socket library supporting TCP protocol on IPv4.
 * @version 0.1
 * @date 2019-10-24
 *
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <vector>

#include "ip.h"
#include "router.h"
#include "socketaddr.h"
#include "tcp.h"

namespace Socket {
constexpr int MSL = 2;

class Socket {
 public:
  // basic information of a socket
  int fd;  // file description id
  SocketAddr src;
  SocketAddr dst;
  Tcp::TcpWorker tcpWorker;

  // information set when created
  int domain;    // only AF_INET supported
  int type;      // only SOCK_STREAM supported
  int protocol;  // such as HTTP

  // lock for basic information
  std::mutex mu;

 public:
  Socket(int domain, int type, int protocol, int fd);
  int bind(const sockaddr* address, socklen_t address_len);
  int listen(int backlog);
  int accept(sockaddr* address, socklen_t* address_len);
  int connect(const sockaddr* address, socklen_t address_len);
  ssize_t read(u_char* buf, size_t nbyte);
  ssize_t write(const u_char* buf, size_t nbyte);
  ssize_t send(Tcp::TcpItem& ti);
  int close();
};
using SocketPtr = std::shared_ptr<Socket>;

class SocketManager {
 public:
  int nextFd = 1024;
  std::vector<SocketPtr> socketList;
  std::map<ip_addr, int> nextPort;

  SocketPtr getSocket(int fd);
  SocketPtr getSocket(const SocketAddr src, const SocketAddr dst);
  SocketPtr getListeningSocket(const SocketAddr src);
  int socket(int domain, int type, int protocol);
  int bind(int socket, const sockaddr* address, socklen_t address_len);
  int listen(int socket, int backlog);
  int accept(int socket, sockaddr* address, socklen_t* address_len);
  int connect(int socket, const sockaddr* address, socklen_t address_len);
  ssize_t read(int fildes, u_char* buf, size_t nbyte);
  ssize_t write(int fildes, const u_char* buf, size_t nbyte);
  int close(int fildes);
};

extern SocketManager sockmgr;

int tcpDispatcher(const void* buf, int len);

}  // namespace Socket

namespace Printer {
void printSocket(const Socket::SocketPtr& sock);
}  // namespace Printer

#endif