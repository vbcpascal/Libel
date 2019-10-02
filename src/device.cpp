#include "device.h"

Device::Device(std::string name) : name(name) {
  id = (max_id++);
  u_char mac[ETHER_ADDR_LEN];

  // get MAC address

#ifndef __APPLE__
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  ifreq ifr;
  strcpy(ifr.ifr_name, name.c_str());
  int res = ioctl(sock, SIOCGIFADD, &ifr);
  for (int i = 0; i < 6; ++i) {
    sprintf(mac + 2 * i, "%02x", (unsigned char)ifr.ifr_hwaddr.sa_data[i]);
  }
  LOG(INFO, "get MAC address: %s\n", mac);
#endif
}

int Device::get_id() { return id; }

std::string Device::get_name() { return name; }

int DeviceController::addDevice(std::string name) {
  auto d = Device(name);
  devices.push_back(d);
  return d.get_id();
}

int DeviceController::findDevice(std::string name) {
  int id = -1;
  for (auto& i : devices) {
    if (i.get_name() == name) id = i.get_id();
  }
  return id;
}

int addDevice(const char* device) {
  LOG(INFO, "Add a new device.");
  int id = deviceCtrl.addDevice(device);
  if (id < 0) {
    return -1;
  }
  LOG(INFO, "Add device succeed. id: %d", id);
  return id;
}

int findDevice(const char* device) {
  int id = deviceCtrl.findDevice(device);
  return id;
}