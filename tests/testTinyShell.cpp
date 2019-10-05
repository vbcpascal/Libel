#include "device.h"
#include "ether.h"
#include "packetio.h"
#include "type.h"

#include <csignal>

std::string cmd;
Device::DevicePtr devPtr = nullptr;

int defaultCallback(const void* buf, int len, int id) {
  // do nothing
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
          "  q exit     Exit.\n\n"
          "  otherwise  Change device. Unless you lie to me!\n");
    }

    else if (cmd == "what" || cmd == "w") {
      printf("id: %d, name: %s, MAC: ", devPtr->getId(),
             devPtr->getName().c_str());
      u_char mac[6];
      devPtr->getMAC(mac);
      MAC::printMAC(mac);
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
      std::string macStr, text;
      u_char buf[512] = {0};

      printf("MAC address: ");
      std::getline(std::cin, macStr);
      printf("Text: ");
      std::getline(std::cin, text);

      u_char mac[6];
      MAC::strtoMAC(mac, macStr.c_str());

      int len = text.length() + 1;
      if (len < ETH_ZLEN) len = ETH_ZLEN;
      sprintf((char*)buf, "%s", text.c_str());
      int res = sendFrame(buf, len, ETHERTYPE_IP, mac, devPtr->getId());
      if (res < 0) {
        LOG_ERR("Send failed");
        continue;
      }

      // local EtherFrame
      EtherFrame frame;
      ether_header hdr;
      memcpy(hdr.ether_dhost, mac, ETHER_ADDR_LEN);
      memcpy(hdr.ether_shost, devPtr->getMAC(), ETHER_ADDR_LEN);
      hdr.ether_type = ETHERTYPE_IP;
      frame.setPayload(buf, len);
      frame.printFrame(2);
    }

    else if (cmd == "exit" || cmd == "q") {
      exit(0);
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
    }
  }
}

int main() {
  signal(SIGINT, signalHandler);
  signal(SIGTSTP, tstpHandler);
  setFrameReceiveCallback(defaultCallback);
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