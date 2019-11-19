#include "api.h"

#define TEST_CLIENT

#ifdef TEST_CLIENT
#define CALL(f, ...) api::socket::f(__VA_ARGS__)
#else
#define CALL(f, ...) f(__VA_ARGS__)
#endif

int main(int argc, char* argv[]) {
  printf(
      "Usage: ./testSocketServer serverIP serverPort\n"
      "  For example: ./testSocketServer 10.100.1.2 4096");
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

  ws = CALL(socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ws < 0) {
    LOG_ERR("socket failed. errno: %d", errno);
  } else {
    LOG_INFO("socket succeed: %d", ws);
  }

  xx = CALL(connect, ws, &dstSock, INET_ADDRSTRLEN);
  if (ws < 0) {
    LOG_ERR("connect failed. errno: %d", errno);
  } else {
    LOG_INFO("connect succeed.");
  }

  xx = CALL(write, ws, sendBuffer, strlen(sendBuffer));
  if (xx < 0) {
    LOG_ERR("write failed. errno: %d", errno)
  } else {
    LOG_INFO("write succeed.");
  }

  xx = CALL(close, ws);
  if (xx < 0) {
    LOG_ERR("close failed. errno: %d", errno)
  } else {
    LOG_INFO("close succeed.");
  }
  return 0;
}