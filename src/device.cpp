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

int initDeviceMACAddr(u_char* mac, const char* if_name = DEFAULT_DEV_NAME) {
#ifdef __APPLE__
  ifaddrs* iflist;
  int found = -1;
  if (getifaddrs(&iflist) == 0) {
    for (ifaddrs* cur = iflist; cur; cur = cur->ifa_next) {
      if ((cur->ifa_addr->sa_family == AF_LINK) &&
          (strcmp(cur->ifa_name, if_name) == 0) && cur->ifa_addr) {
        auto sdl = reinterpret_cast<sockaddr_dl*>(cur->ifa_addr);
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

std::pair<ip_addr, ip_addr> initDeviceIPAddr(const char* if_name) {
  ip_addr ipAddr, mask;
  ifaddrs* iflist;

  if (getifaddrs(&iflist) == 0) {
    for (ifaddrs* cur = iflist; cur; cur = cur->ifa_next) {
      if ((cur->ifa_addr->sa_family == AF_INET) &&
          (strcmp(cur->ifa_name, if_name) == 0) && cur->ifa_addr) {
        auto sdl = reinterpret_cast<sockaddr_in*>(cur->ifa_addr);
        ipAddr = sdl->sin_addr;
        sdl = reinterpret_cast<sockaddr_in*>(cur->ifa_netmask);
        mask = sdl->sin_addr;
        break;
      }
    }
    freeifaddrs(iflist);
  }

  return std::make_pair(ipAddr, mask);
}

void getPacket(u_char* args, const struct pcap_pkthdr* header,
               const u_char* packet) {
  // args may be useless?(10.2)
  // NO!!(10.3)
  pcapArgs* pa = reinterpret_cast<pcapArgs*>(args);

  int len = header->len;
  if (len != header->caplen) {
    LOG_ERR("Data Lost.");
    return;
  }

  if (callback != nullptr) {
    int res = callback(packet, len, pa->id);
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
  if (initDeviceMACAddr(mac, name.c_str()) < 0) {
    LOG_WARN("get MAC address failed. name: \033[1m%s\033[0m", name.c_str());
    badDevice();
    return;
  }

  // get IP
  auto ipm = initDeviceIPAddr(name.c_str());
  ip = ipm.first;
  subnetMask = ipm.second;
  if (ip.s_addr == 0) {
    LOG_WARN("get Ip address failed. name: \033[1m%s\033[0m", name.c_str());
    badDevice();
    return;
  }

  // obtain a PCAP descriptor
  char pcap_errbuf[PCAP_ERRBUF_SIZE];
  memset(pcap_errbuf, 0, PCAP_ERRBUF_SIZE);
  // pcap = pcap_create(name.c_str(), pcap_errbuf);
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
  startSending();
}

DeviceId Device::getId() { return id; }

std::string Device::getName() { return name; }

void Device::getMAC(u_char* dst_mac) {
  std::memcpy(dst_mac, mac, ETHER_ADDR_LEN);
}

const u_char* Device::getMAC() { return mac; }

ip_addr Device::getIp() { return ip; }

ip_addr Device::getSubnetMask() { return subnetMask; }

int Device::sendFrame(Ether::EtherFrame& frame) {
  sender.push(frame);
  return 0;
}

int Device::startSniffing() {
  if (sniffing) return -1;

  sniffing = true;
  pcapArgs* pa = new pcapArgs(id, name, mac);
  if (!pcap) {
    LOG_ERR("No pcap.");
    return -1;
  }
  sniffingThread = std::thread(
      [=]() { pcap_loop(pcap, -1, getPacket, reinterpret_cast<u_char*>(pa)); });
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

int Device::startSending() {
  sendingThread = std::thread([&]() { senderLoop(); });
  sendingThread.detach();
  return 0;
}

void Device::senderLoop() {
  std::unique_lock<std::mutex> lk(cv_m);

  while (true) {
    cv.wait(lk, [&]() { return sender.size() > 0; });
    while (sender.size()) {
      auto frame = sender.front();
      sender.pop();
      frame.htonType();

      // send the ethernet frame
      if (pcap_inject(pcap, frame.getFrame(), frame.getLength()) == -1) {
        pcap_perror(pcap, 0);
        LOG_ERR("Send frame failed.");
      }
    }
  }
}

//////////////////// DeviceManager ////////////////////

DeviceId DeviceManager::addDevice(std::string name, bool sniff) {
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

  char ipstr[20], maskstr[20];
  strcpy(ipstr, inet_ntoa(dev->getIp()));
  strcpy(maskstr, inet_ntoa(dev->getSubnetMask()));
  LOG_INFO("id: %d, mac: %s, ip: %s, mask: %s, name: \033[33;1m%s\033[0m", id,
           MAC::toString(mac).c_str(), ipstr, maskstr, dev->getName().c_str());
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

DevicePtr DeviceManager::getDevicePtr(const ip_addr& _ip) {
  DevicePtr devPtr = nullptr;
  for (auto& dev : devices) {
    if (dev->getIp() == _ip) {
      devPtr = dev;
      break;
    }
  }
  return devPtr;
}

bool DeviceManager::haveDeviceWithIp(const ip_addr& ip) {
  for (auto& dev : devices) {
    if (dev->getIp() == ip) {
      return true;
    }
  }
  return false;
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

  pcap_freealldevs(devsPtr);
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

int DeviceManager::sendFrame(DevicePtr dev, Ether::EtherFrame& frame) {
  dev->getMAC(frame.frame.header.ether_shost);
  int res = dev->sendFrame(frame);
  if (res < 0) {
    LOG_ERR("Sending frame failed. error code: %d", res);
    return -1;
  }
  // LOG_INFO("Sending frame succeed.");
  return 0;
}

int DeviceManager::sendFrame(DeviceId id, Ether::EtherFrame& frame) {
  int found = -1;
  for (auto& dev : devices) {
    if (dev->getId() == id) {
      found = 1;
      return sendFrame(dev, frame);
    }
  }
  return found;
}

int DeviceManager::sendFrame(const void* buf, int len, int ethtype,
                             const void* destmac, DevicePtr dev) {
  if ((len + ETHER_HDR_LEN) > ETHER_MAX_LEN) {
    LOG(ERR, "len is too large: %d.", len);
    return -1;
  }

  ether_header hdr;
  hdr.ether_type = (u_short)ethtype;
  std::memcpy(hdr.ether_dhost, destmac, ETHER_ADDR_LEN);

  Ether::EtherFrame frame;
  frame.setHeader(hdr);
  frame.setPayload((u_char*)buf, len);
  return sendFrame(dev, frame);
}

int DeviceManager::sendFrame(const void* buf, int len, int ethtype,
                             const void* destmac, DeviceId id) {
  if ((len + ETHER_HDR_LEN) > ETHER_MAX_LEN) {
    LOG(ERR, "len is too large: %d.", len);
    return -1;
  }

  DevicePtr dev = getDevicePtr(id);
  return sendFrame(buf, len, ethtype, destmac, dev);
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
