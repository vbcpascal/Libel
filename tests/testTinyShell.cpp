/**
 * @file testTinyShell.cpp
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-10-02
 *
 * @brief Test: not maintained
 *
 */

#include <csignal>

#include "api.h"
#include "device.h"
#include "ether.h"
#include "type.h"

std::string cmd;
Device::DevicePtr devPtr = nullptr;
bool showMsg = false;

int defaultCallback(const void* buf, int len, DeviceId id) {
  if (showMsg) {
    const char* cstr = (const char*)buf;
    LOG_INFO("Message: %s", cstr);
  }
  return 0;
}

int fullCallback(const void* buf, int len, DeviceId id) {
  Ether::EtherFrame frame;
  frame.setPayload((u_char*)buf, len);
  Printer::printEtherFrame(frame, 2, e_PRINT_NONE);

  if (showMsg) {
    const char* cstr = (const char*)buf;
    LOG_INFO("Message: %s", cstr);
  }
  return 0;
}

void signalHandler(int signum) {
  // printf("\n[ %d ] signal got\n", signum);
  if (devPtr) {
    devPtr->stopSniffing();
  }
  printf("\n>>> ");
  fflush(stdout);
}

void tstpHandler(int signum) { exit(0); }

int cmdHandler() {
  std::string dstMAC = "ff:ff:ff:ff:ff:ff";
  while (true) {
    printf(">>> ");
    fflush(stdout);
    std::getline(std::cin, cmd);

    if (cmd == "") {
      continue;
    }

    else if (cmd == "help" || cmd == "h") {
      printf(
          "help page:\n\n"
          "  w what     Show basic information of currnet device.\n"
          "  l sniff    Begin to sniff on current device.\n"
          "  s send     Send an easy frame on current device.\n"
          "  f full     Show full information when receiving a packet.\n"
          "  b brief    Show brief information when receiving a packet.\n"
          "  o show     Show/[Hide] frame detail sent."
          "  q exit     Exit.\n\n"
          "  otherwise  Change device. Unless you lie to me!\n");
    }

    else if (cmd == "what" || cmd == "w") {
      printf("id: %d, name: %s, MAC: ", devPtr->getId(),
             devPtr->getName().c_str());
      u_char mac[6];
      devPtr->getMAC(mac);
      Printer::printMAC(mac);
      continue;
    }

    else if (cmd == "sniff" || cmd == "l") {
      if (!devPtr) {
        LOG_ERR("please input a device name before you sniffing.");
        continue;
      }
      if (devPtr->startSniffing() < 0) {
        LOG_ERR("Sniffing error on device %s", devPtr->getName().c_str());
        continue;
      } else {
        LOG_INFO("Begin to sniff device %s", devPtr->getName().c_str());
        if (devPtr->sniffingThread.joinable()) devPtr->sniffingThread.join();
      }
    }  // sniff

    else if (cmd == "send" || cmd == "s") {
      std::string dMAC, text;
      u_char buf[512] = {0};

      printf("MAC address: ");
      std::getline(std::cin, dMAC);
      if (dMAC == "") {
        LOG_INFO("Got an empty line. Use last address instead: %s",
                 dMAC.c_str());
      } else {
        dstMAC = dMAC;
      }

      printf("Text: ");
      std::getline(std::cin, text);

      u_char mac[6];
      MAC::strtoMAC(mac, dstMAC.c_str());

      int len = text.length() + 1;
      if (len < ETH_ZLEN) len = ETH_ZLEN;
      sprintf((char*)buf, "%s", text.c_str());
      int res = api::sendFrame(buf, len, ETHERTYPE_IP, mac, devPtr->getId());
      if (res < 0) {
        LOG_ERR("Send failed");
        continue;
      }

      // local EtherFrame
      if (showMsg) {
        Ether::EtherFrame frame;
        ether_header hdr;
        memcpy(hdr.ether_dhost, mac, ETHER_ADDR_LEN);
        memcpy(hdr.ether_shost, devPtr->getMAC(), ETHER_ADDR_LEN);
        hdr.ether_type = ETHERTYPE_IP;
        frame.setHeader(hdr);
        frame.setPayload(buf, len);
        Printer::printEtherFrame(frame, 2);
      }
    }

    else if (cmd == "brief" || cmd == "b") {
      api::setFrameReceiveCallback(defaultCallback);
      continue;
    }

    else if (cmd == "full" || cmd == "f") {
      api::setFrameReceiveCallback(fullCallback);
      continue;
    }

    else if (cmd == "exit" || cmd == "q") {
      exit(0);
    }

    else if (cmd == "show" || cmd == "o") {
      showMsg = !showMsg;
      LOG_INFO("Show frame sent: %s", showMsg ? "Show" : "Hide");
      continue;
    }

    else {
      auto dev = Device::deviceMgr.getDevicePtr(cmd);
      if (dev) {
        if (devPtr) devPtr->stopSniffing();
        devPtr = dev;
        LOG_INFO("Set device succeed. Try to use \"send\" or \"sniff\"!");
      } else {
        LOG_ERR("Unknown device or command: %s", cmd.c_str());
      }
      continue;
    }
  }
}

int main() {
  signal(SIGINT, signalHandler);
  signal(SIGTSTP, tstpHandler);
  api::setFrameReceiveCallback(defaultCallback);
  int cnt = Device::deviceMgr.addAllDevice(false);

  if (cnt <= 0) {
    LOG_INFO("No device added. Exit");
    exit(0);
  }

  devPtr = Device::deviceMgr.getDevicePtr(0);
  LOG_INFO(
      "Device listed. Try to input the name of a device. default device: "
      "\033[33;1m%s\033[0m",
      devPtr->getName().c_str())
  cmdHandler();

  return 0;
}
