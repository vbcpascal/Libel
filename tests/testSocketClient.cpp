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
      fflush(stdout);                              \
      return -1;                                   \
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
      "Usage: ./testSocketClient serverIP serverPort\n"
      "  For example: ./testSocketClient 10.100.1.2 4096\n");
  char* addrStr = argv[argc - 2];
  char* portStr = argv[argc - 1];

  ip_addr ip;
  if (inet_aton(addrStr, &ip) <= 0) {
    LOG_ERR("Ip address error: %s", argv[2]);
    return 0;
  }
  in_port_t port = std::atoi(portStr);

  LOG_INFO("Will connect to %s:%d", addrStr, port);
  sockaddr dstSock;
  Socket::SocketAddr(ip, port).get(&dstSock);

  char sendBuffer[100] =
      "Good day, viewers. In this segment, we will test our TCP/IP protocol.";
  int xx, ws;

  RUN_FUNC(ws, socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
  RUN_FUNC(xx, connect, ws, &dstSock, sizeof(sockaddr_in));
  RUN_FUNC(xx, write, ws, sendBuffer, strlen(sendBuffer));
  RUN_FUNC(xx, close, ws);

  return 0;
}
