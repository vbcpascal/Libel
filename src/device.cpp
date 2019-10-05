#include "device.h"

namespace Device {

DeviceManager deviceMgr;
frameReceiveCallback callback;

struct pcapArgs {
  DeviceId id;
  std::string name;
  u_char mac[ETHER_ADDR_LEN];

  pcapArgs(DeviceId id, std::string name, u_char* m) : id(id), name(name) {
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
    LOG_ERR("Data Lost.");
    return;
  }

  auto frame = EtherFrame(packet, header->len);
  if (frame.len == 0) return;
  frame.header.ether_type = ntohs(frame.header.ether_type);
  frame.updateHeader();

  printf("id: %d\t", pa->id);
  if (frame.len) frame.printFrame();

  if (callback != nullptr) {
    int res = callback(frame.getPayload(), frame.getPayloadLength(), pa->id);
    if (res < 0) {
      LOG_ERR("Callback error!");
    }
  }
}

//////////////////// Device ////////////////////

DeviceId Device::max_id = 0;

void Device::badDevice() {
  --max_id;
  id = -1;
}

Device::~Device() {
  stopSniffing();
  if (pcap) pcap_close(pcap);
}

Device::Device(std::string name, bool sniff)
    : name(name), pcap(nullptr), sniffing(false) {
  id = (max_id++);

  // get MAC
  if (getMACAddr(mac, name.c_str()) < 0) {
    LOG_WARN("get MAC address failed. name: \033[1m%s\033[0m", name.c_str());
    badDevice();
    return;
  }

  // obtain a PCAP descriptor
  char pcap_errbuf[PCAP_ERRBUF_SIZE];
  memset(pcap_errbuf, 0, PCAP_ERRBUF_SIZE);
  pcap = pcap_open_live(name.c_str(), MAX_FRAME_SIZE, false, FRAME_TIME_OUT,
                        pcap_errbuf);
  if (pcap_errbuf[0] != '\0') {
    LOG_WARN("pcap_open_live error! name: \033[1m%s\033[0m: %s", name.c_str(),
             pcap_errbuf);
    badDevice();
    return;
  }
  if (!pcap) {
    LOG_WARN("Cannot get pcap. name: \033[1m%s\033[0m", name.c_str());
    badDevice();
    return;
  }

  // start sniffing
  if (sniff) startSniffing();
}

DeviceId Device::getId() { return id; }

std::string Device::getName() { return name; }

void Device::getMAC(u_char* dst_mac) {
  std::memcpy(dst_mac, mac, ETHER_ADDR_LEN);
}

const u_char* Device::getMAC() { return mac; }

int Device::sendFrame(EtherFrame& frame) {
  LOG_INFO("Sending Frame in device %s with id %d.", name.c_str(), id);

  frame.header.ether_type = htons(frame.header.ether_type);
  frame.updateHeader();

  // send the ethernet frame
  if (pcap_inject(pcap, frame.frame, frame.len) == -1) {
    pcap_perror(pcap, 0);
    // pcap_close(pcap);
    return -3;
  }
  return 0;
}

int Device::startSniffing() {
  if (sniffing) return -1;

  sniffing = true;
  pcapArgs* pa = new pcapArgs(id, name, mac);
  sniffingThread =
      std::thread([&]() { pcap_loop(pcap, -1, getPacket, (u_char*)pa); });
  return 0;
}

int Device::stopSniffing() {
  if (!sniffing) return -1;

  sniffing = false;
  pthread_t pthread = sniffingThread.native_handle();
  if (pthread_cancel(pthread)) return -1;
  sniffingThread.detach();
  return 0;
}

//////////////////// DeviceManager ////////////////////

DeviceId DeviceManager::addDevice(std::string name, bool sniff) {
  // LOG_INFO("Add a new device, name: \033[33;1m%s\033[0m", name);
  if (findDevice(name) >= 0) {
    LOG_WARN("Device exists, no actions.");
    return -1;
  }

  DevicePtr dev = std::make_shared<Device>(name, sniff);
  DeviceId id = dev->getId();
  if (id < 0) {
    return -1;
  }

  u_char mac[ETHER_ADDR_LEN];
  dev->getMAC(mac);

  devices.push_back(dev);
  LOG(INFO,
      "id: %d, mac: %02x:%02x:%02x:%02x:%02x:%02x. name: "
      "\033[33;1m%s\033[0m",
      id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
      dev->getName().c_str());
  return id;
}

DeviceId DeviceManager::findDevice(std::string name) {
  DeviceId id = -1;
  for (auto& dev : devices) {
    if (dev->getName() == name) {
      id = dev->getId();
      break;
    }
  }
  return id;
}

DevicePtr DeviceManager::getDevicePtr(DeviceId id) {
  DevicePtr devPtr = nullptr;
  for (auto& dev : devices) {
    if (dev->getId() == id) {
      devPtr = dev;
      break;
    }
  }
  return devPtr;
}

DevicePtr DeviceManager::getDevicePtr(std::string name) {
  DevicePtr devPtr = nullptr;
  for (auto& dev : devices) {
    if (dev->getName() == name) {
      devPtr = dev;
      break;
    }
  }
  return devPtr;
}

int DeviceManager::addAllDevice(bool sniff) {
  int cnt = 0;

  pcap_if_t* devsPtr;
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_findalldevs(&devsPtr, errbuf);

  while (devsPtr != nullptr) {
    DeviceId id = addDevice(devsPtr->name, sniff);
    if (id >= 0) ++cnt;
    devsPtr = devsPtr->next;
  }

  return cnt;
}

int DeviceManager::getMACAddr(u_char* mac, DeviceId id) {
  int found = -1;
  for (auto& dev : devices) {
    if (dev->getId() == id) {
      found = 1;
      dev->getMAC(mac);
      break;
    }
  }
  return found;
}

int DeviceManager::sendFrame(DeviceId id, EtherFrame& frame) {
  int found = -1;
  for (auto& dev : devices) {
    if (dev->getId() == id) {
      found = 1;
      dev->getMAC(frame.header.ether_shost);
      frame.updateHeader();
      int res = dev->sendFrame(frame);
      if (res < 0) {
        LOG_ERR("Sending frame failed. error code: %d", res);
        return -1;
      }
      LOG_INFO("Sending frame succeed.");
      break;
    }
  }
  return found;
}

int DeviceManager::sendFrame(const void* buf, int len, int ethtype,
                             const void* destmac, DeviceId id) {
  if (!ETHER_IS_VALID_LEN(len + ETHER_HDR_LEN)) {
    LOG(ERR, "len is too large: %d.", len);
    return -1;
  }

  ether_header hdr;
  hdr.ether_type = (u_short)ethtype;
  std::memcpy(hdr.ether_dhost, destmac, ETHER_ADDR_LEN);

  EtherFrame frame;
  frame.setHeader(hdr);
  frame.setPayload(buf, len);
  return sendFrame(id, frame);
}

int DeviceManager::keepReceiving() {
  for (auto& dev : devices) {
    if (dev->sniffingThread.joinable()) {
      dev->sniffingThread.join();
    }
  }
  return 0;
}

}  // namespace Device

//////////////////// API ////////////////////

int addDevice(const char* device) {
  DeviceId id = -1;
  id = Device::deviceMgr.addDevice(device);

  return id;
}

int findDevice(const char* device) {
  DeviceId id = Device::deviceMgr.findDevice(device);
  return id;
}

int keepReceiving() { return Device::deviceMgr.keepReceiving(); }