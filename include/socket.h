/**
 * @file socket.h
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-10-24
 *
 * @brief POSIX-compatible socket library supporting TCP protocol on IPv4.
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
/**
 * @brief MSL in TCP protocol
 *
 */
constexpr int MSL = 2;

/**
 * @brief A socket class supporting basic information and functions.
 *
 */
class Socket {
 public:
  // basic information of a socket

  /**
   * @brief file description id
   *
   */
  int fd;

  /**
   * @brief A related tcp worker to send or handle segments with more
   * information of a socket.
   *
   */
  Tcp::TcpWorker tcpWorker;
  SocketAddr src;
  SocketAddr dst;

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

/**
 * @brief A manager managing all sockets
 *
 */
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

/**
 * @brief Dispatch tcp segments to related socket
 *
 */
int tcpDispatcher(const void* buf, int len);

}  // namespace Socket

namespace Printer {
/**
 * @brief Print some basic information of a socket
 *
 * @param sock socket to print
 */
void printSocket(const Socket::SocketPtr& sock);
}  // namespace Printer

#endif