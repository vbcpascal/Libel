/**
 * @file testSendFrame.cpp
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-11-19
 *
 * @brief Test: open one device and send 100 frames to destination MAC address.
 * It is necessary to set device with the same program on the dst host.
 *
 */

#include <csignal>

#include "api.h"

int count = 0;

int defaultCallback(const void* buf, int len, DeviceId id) {
  Ether::EtherFrame ef(buf, len);
  char msg[200];
  strncpy(msg, reinterpret_cast<const char*>(ef.frame.payload), 200);
  printf("Message: (%d) %s", count++, msg);

  return 0;
}

int main() {
  api::setFrameReceiveCallback(defaultCallback);
  int cnt = Device::deviceMgr.addAllDevice(false);

  if (cnt <= 0) {
    LOG_INFO("No device added. Exit");
    exit(0);
  }

  std::string devName;
  std::cout << "Please input a device to send and receive frames: ";
  std::cin >> devName;
  auto dev = Device::deviceMgr.getDevicePtr(devName.c_str());
  if (!dev) {
    LOG_ERR("Unknown device: %s", devName.c_str());
  }
  dev->startSniffing();

  u_char mac[6];
  std::string dstMAC;
  std::cout << "Please input destnation MAC address: ";
  std::cin >> dstMAC;
  MAC::strtoMAC(mac, dstMAC.c_str());

  LOG_INFO("Will send 100 frames to: %s", dstMAC.c_str());

  std::string text = "Good day viewers";
  u_char buf[512] = {0};

  int len = text.length() + 1;
  if (len < ETH_ZLEN) len = ETH_ZLEN;
  sprintf((char*)buf, "%s", text.c_str());

  for (int i = 0; i < 100; ++i) {
    int res = api::sendFrame(buf, len, ETHERTYPE_IP, mac, dev->getId());
    if (res < 0) {
      LOG_ERR("Send failed: %d", i);
    }
  }

  // keepReceiving();
  return 0;
}