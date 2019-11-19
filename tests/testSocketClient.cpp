#include "api.h"

#define TEST_CLIENT

#ifdef TEST_CLIENT
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

  LOG_INFO("Will connect to %s:%d", addrStr, port);
  sockaddr dstSock;
  Socket::SocketAddr(ip, port).get(&dstSock);

  char sendBuffer[100] =
      "Good day, views. In this segment, we will test our TCP/IP protocol in "
      "client.";
  int xx, ws;

  RUN_FUNC(ws, socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
  RUN_FUNC(xx, connect, ws, &dstSock, INET_ADDRSTRLEN);
  RUN_FUNC(xx, write, ws, sendBuffer, strlen(sendBuffer));
  RUN_FUNC(xx, close, ws);

  return 0;
}