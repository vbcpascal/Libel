#include <cstdlib>
#include "api.h"

#define TEST_SERVER

#ifdef TEST_SERVER
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

  char recvBuffer[100];
  int xx, ls, ws;

  RUN_FUNC(ls, socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
  RUN_FUNC(xx, bind, ls, &srcSock, INET_ADDRSTRLEN);
  RUN_FUNC(xx, listen, ls, 10);
  RUN_FUNC(ws, accept, ls, &dstSock, &dstSockLen);
  RUN_FUNC(xx, read, ws, recvBuffer, 20);
  RUN_FUNC(xx, close, ws);

  printf("Text received: %s", recvBuffer);
  return 0;
}