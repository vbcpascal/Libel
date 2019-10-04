#include "device.h"

namespace Device {

DeviceController deviceCtrl;
frameReceiveCallback callback;

struct pcapArgs {
  int id;
  std::string name;
  u_char mac[ETHER_ADDR_LEN];

  pcapArgs(int id, std::string name, u_char* m) : id(id), name(name) {
    std::memcpy(mac, m, ETHER_ADDR_LEN);
  }
};

int getMACAddr(u_char* mac, const char* if_name = DEFAULT_DEV_NAME) {
#ifdef __APPLE__
  ifaddrs* iflist;
  int found = -1;
  if (getifaddrs(&iflist) == 0) {
    for (ifaddrs* cur = iflist; cur; cur = cur->ifa_next) {
      if ((cur->ifa_addr->sa_family == AF_LINK) &&
          (strcmp(cur->ifa_name, if_name) == 0) && cur->ifa_addr) {
        sockaddr_dl* sdl = (sockaddr_dl*)cur->ifa_addr;
        memcpy(mac, LLADDR(sdl), sdl->sdl_alen);
        found = 1;
        break;
      }
    }
    freeifaddrs(iflist);
  }
  return found;

#else
  ifreq ifinfo;
  int found = -1;
  strcpy(ifinfo.ifr_name, if_name);
  int sd = socket(AF_INET, SOCK_DGRAM, 0);
  int res = ioctl(sd, SIOCGIFHWADDR, &ifinfo);
  close(sd);

  if ((res == 0) && (ifinfo.ifr_hwaddr.sa_family == 1)) {
    memcpy(mac, ifinfo.ifr_hwaddr.sa_data, IFHWADDRLEN);
    found = 1;
  }
  return found;
#endif
}

void getPacket(u_char* args, const struct pcap_pkthdr* header,
               const u_char* packet) {
  // args may be useless?(10.2)
  // NO!!(10.3)
  pcapArgs* pa = (pcapArgs*)args;

  if (header->len != header->caplen) {
    LOG(ERR, "Data Lost.");
    return;
  }

  auto frame = EtherFrame(packet, header->len);
  if (frame.len == 0) return;

  printf("id: %d\t", pa->id);
  if (frame.len) frame.printFrame();

  if (callback != NULL) {
    int res = callback(frame.getPayload(), frame.getPayloadLength(), pa->id);
    if (res < 0) {
      LOG(ERR, "Callback error!");
    }
  }
}

//////////////////// Device ////////////////////

void Device::badDevice() {
  --max_id;
  id = -1;
}

Device::~Device() {
  if (sniffing && sniffingThread.joinable()) sniffingThread.join();
}

Device::Device(std::string name, bool sniff) : name(name), sniffing(false) {
  id = (max_id++);

  // get MAC
  if (getMACAddr(mac, name.c_str()) < 0) {
    LOG(ERR, "get MAC address failed.");
    badDevice();
    return;
  }
  // LOG(INFO, "get MAC address succeed.");
  // EtherFrame::printMAC(mac);

  // obtain a PCAP descriptor
  char pcap_errbuf[PCAP_ERRBUF_SIZE];
  memset(pcap_errbuf, 0, PCAP_ERRBUF_SIZE);
  pcap = pcap_open_live(name.c_str(), MAX_FRAME_SIZE, false, FRAME_TIME_OUT,
                        pcap_errbuf);
  if (pcap_errbuf[0] != '\0') {
    LOG(ERR, "pcap_open_live error! %s", pcap_errbuf);
    badDevice();
    return;
  }
  if (!pcap) {
    LOG(ERR, "Cannot get pcap.");
    badDevice();
    return;
  }

  // start sniffing
  if (sniff) startSniffing();
}

int Device::getId() { return id; }

std::string Device::getName() { return name; }

void Device::getMAC(u_char* dst_mac) {
  std::memcpy(dst_mac, mac, ETHER_ADDR_LEN);
}

int Device::sendFrame(EtherFrame& frame) {
  LOG(INFO, "Sending Frame in device %s with id %d.", name.c_str(), id);

  // send the ethernet frame
  if (pcap_inject(pcap, frame.frame, frame.len) == -1) {
    pcap_perror(pcap, 0);
    pcap_close(pcap);
    return -3;
  }
  pcap_close(pcap);
  return 0;
}

int Device::startSniffing() {
  if (sniffing) return -1;

  sniffing = true;
  pcapArgs pa(id, name, mac);
  sniffingThread =
      std::thread([&]() { pcap_loop(pcap, -1, getPacket, (u_char*)&pa); });
  return 0;
}

//////////////////// DeviceController ////////////////////

int DeviceController::addDevice(std::string name) {
  DevicePtr dev = std::make_shared<Device>(name);
  int id = dev->getId();
  if (id < 0) {
    return -1;
  }

  devices.push_back(dev);
  LOG(INFO, "Add device succeed. name: %s, id: %d", dev->getName().c_str(), id);
  return id;
}

int DeviceController::findDevice(std::string name) {
  int id = -1;
  for (auto& dev : devices) {
    if (dev->getName() == name) id = dev->getId();
  }
  return id;
}

int DeviceController::getMACAddr(u_char* mac, int id) {
  int found = -1;
  for (auto& dev : devices) {
    if (dev->getId() == id) {
      found = 1;
      dev->getMAC(mac);
    }
  }
  return found;
}

int DeviceController::sendFrame(int id, EtherFrame& frame) {
  int found = -1;
  for (auto& dev : devices) {
    if (dev->getId() == id) {
      found = 1;
      dev->getMAC(frame.header.ether_shost);
      frame.updateHeader();
      int res = dev->sendFrame(frame);
      if (res < 0) {
        LOG(ERR, "Sending frame failed. error code: %d", res);
        return -1;
      }
      LOG(INFO, "Sending frame succeed.");
      break;
    }
  }
  return found;
}

int DeviceController::keepReceiving() {
  for (auto& dev : devices) {
    if (dev->sniffingThread.joinable()) {
      dev->sniffingThread.join();
    }
  }
  return 0;
}

}  // namespace Device

int addDevice(const char* device) {
  LOG(INFO, "Add a new device, name: \033[33;1m%s\033[0m", device);
  int id = -1;
  id = Device::deviceCtrl.addDevice(device);

  return id;
}

int findDevice(const char* device) {
  int id = Device::deviceCtrl.findDevice(device);
  return id;
}

int keepReceiving() { return Device::deviceCtrl.keepReceiving(); }