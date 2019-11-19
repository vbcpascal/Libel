#include <cstdlib>
#include "api.h"

#define TEST_SERVER

#ifdef TEST_SERVER
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

  LOG_INFO("Build a server with %s:%d", addrStr, port);
  sockaddr srcSock, dstSock;
  socklen_t dstSockLen;
  Socket::SocketAddr(ip, htonl(port)).get(&srcSock);

  char recvBuffer[100];
  int xx, ls, ws;

  ls = CALL(socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ls < 0) {
    LOG_ERR("socket failed. errno: %d", errno);
  } else {
    LOG_INFO("socket succeed: %d", ls);
  }

  xx = CALL(bind, ls, &srcSock, INET_ADDRSTRLEN);
  if (xx < 0) {
    LOG_ERR("bind failed. errno: %d", errno);
  } else {
    LOG_INFO("bind succeed.");
  }

  xx = CALL(listen, ls, 10);
  if (xx < 0) {
    LOG_ERR("listen failed. errno: %d", errno);
  } else {
    LOG_INFO("listen succeed.");
  }

  ws = CALL(accept, ls, &dstSock, &dstSockLen);
  if (ws < 0) {
    LOG_ERR("accept failed. errno: %d", errno);
  } else {
    LOG_INFO("accept succeed: %d", ws);
  }

  xx = CALL(read, ws, recvBuffer, 20);
  if (xx < 0) {
    LOG_ERR("read failed. errno: %d", errno);
  } else {
    LOG_INFO("read succeed.");
  }

  xx = CALL(close, ws);
  if (xx < 0) {
    LOG_ERR("close failed. errno: %d", errno);
  } else {
    LOG_INFO("close succeed.");
  }

  printf("Text received: %s", recvBuffer);
  return 0;
}