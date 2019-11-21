/**
 * @file testSocketServer.cpp
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-11-21
 *
 * @brief Test: test socket as a server. It will call functions of api::socket
 * if TEST_SERVER defined. Or it will call the functions of system.
 *
 */

#include <cstdlib>
#include "api.h"

#define TEST_API

#ifdef TEST_API
#define CALL(f, ...) api::socket::f(__VA_ARGS__)
#else
#define CALL(f, ...) f(__VA_ARGS__)
#endif

#define RUN_FUNC(var, func, ...)                   \
  {                                                \
    var = CALL(func, __VA_ARGS__);                 \
    if (var < 0) {                                 \
      LOG_ERR(#func " failed. errno: %d", errno);  \
    } else {                                       \
      LOG_INFO(#func " succeed. return: %d", var); \
    }                                              \
  }

int main(int argc, char* argv[]) {
#ifdef TEST_API
  printf("\033[32;1m === Test Server: API ===\033[0m\n");
#else
  printf("\033[32;1m === Test Server: System ===\033[0m\n");
#endif

  printf(
      "Usage: ./testSocketServer serverIP serverPort\n"
      "  For example: ./testSocketServer 10.100.1.2 4096\n");
  char* addrStr = argv[argc - 2];
  char* portStr = argv[argc - 1];

  ip_addr ip;
  if (inet_aton(addrStr, &ip) <= 0) {
    LOG_ERR("Ip address error: %s", argv[2]);
    return 0;
  }
  int port = std::atoi(portStr);

  LOG_INFO("Build a server with %s:%d", addrStr, port);
  sockaddr srcSock, dstSock;
  socklen_t dstSockLen;
  Socket::SocketAddr(ip, port).get(&srcSock);

  Socket::SocketAddr sa(&srcSock);

  char recvBuffer[100];
  int xx, ls, ws;

  RUN_FUNC(ls, socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
  RUN_FUNC(xx, bind, ls, &srcSock, sizeof(sockaddr_in));
  RUN_FUNC(xx, listen, ls, 10);
  RUN_FUNC(ws, accept, ls, &dstSock, &dstSockLen);
  RUN_FUNC(xx, read, ws, recvBuffer, 100);
  printf("\033[;1mText received\033[0m: \033[33m%s\033[0m\n", recvBuffer);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  RUN_FUNC(xx, close, ws);

  return 0;
}