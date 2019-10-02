/**
 * @file device.h
 * @author guanzhichao
 * @brief Library supporting network device management.
 * @version 0.1
 * @date 2019-10-02
 *
 */
#ifndef DEVICE_H
#define DEVICE_H

#include <net/if.h>
#include <pcap/pcap.h>
#include <sys/ioctl.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ether.h"
#include "type.h"

static int max_id = 0;

class Device {
 private:
  int id;
  std::string name;

 public:
  Device(std::string name);
  int get_id();
  std::string get_name();
};

using DevicePtr = std::shared_ptr<Device>;

class DeviceController {
 private:
  std::vector<Device> devices;

 public:
  int addDevice(std::string name);
  int findDevice(std::string name);
  int sendFrame(EtherFrame frame);
};

DeviceController deviceCtrl;

/**
 * Add a device to the library for sending/receiving packets.
 *
 * @param device Name of network device to send/receive packet on.
 * @return A non-negative _device-ID_ on success, -1 on error.
 */
int addDevice(const char *device);

/**
 * Find a device added by `addDevice`.
 *
 * @param device Name of the network device.
 * @return A non-negative _device-ID_ on success, -1 if no such device
 * was found.
 */
int findDevice(const char *device);

#endif