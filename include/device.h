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

#include <ifaddrs.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ether.h"
#include "type.h"

// 0 means no time out
#define FRAME_TIME_OUT 10
#define MAX_FRAME_SIZE 65536

namespace Device {

/**
 * @brief Device created by addDevice
 *
 */
class Device {
 public:
  /**
   * @brief thread sniffing
   *
   */
  std::thread sniffingThread;

  ~Device();

  /**
   * @brief Construct a new Device object
   *
   * @param name the name of device
   * @param sniff whether begin to sniff after construct this device
   */
  Device(std::string name, bool sniff = false);

  /**
   * @brief Get the Id object
   *
   * @return int the id
   */
  int getId();

  /**
   * @brief Get the Name object
   *
   * @return std::string the name
   */
  std::string getName();

  /**
   * @brief Get the MAC address
   *
   * @param dst_mac MAC address will be stored in
   */
  void getMAC(u_char *dst_mac);

  /**
   * @brief Get the MAC address (Not Recommand!)
   *
   * @return const u_char* MAC address
   */
  const u_char *getMAC();

  /**
   * @brief Send a frame on the device
   *
   * @param frame the frame will be sent
   * @return int 0 on success, -1 on error
   */
  int sendFrame(EtherFrame &frame);

  /**
   * @brief start sniffing in this device
   *
   * @return int 0 on success, -1 on error
   */
  int startSniffing();

  /**
   * @brief stop sniffing
   *
   * @return int 0 on success, -1 on error
   */
  int stopSniffing();

 private:
  static int max_id;
  int id;
  std::string name;
  u_char mac[ETHER_ADDR_LEN];

  pcap_t *pcap;
  bool sniffing;

  void badDevice();
};

using DevicePtr = std::shared_ptr<Device>;

/**
 * @brief Control all the devices.
 *
 */
class DeviceManager {
 private:
  std::vector<DevicePtr> devices;

 public:
  /**
   * @brief Add a new device
   *
   * @param name the name of device
   * @return int id, -1 on error
   */
  int addDevice(std::string name, bool sniff = true);

  /**
   * @brief Find a device
   *
   * @param name the name of device
   * @return int id, -1 on error
   */
  int findDevice(std::string name);

  /**
   * @brief Get the pointer of device according to id
   *
   * @param id id of device
   * @return DevicePtr pointer of device
   */
  DevicePtr getDevicePtr(int id);

  /**
   * @brief Get the pointer of device according to id
   *
   * @param name name of device
   * @return DevicePtr pointer of device
   */
  DevicePtr getDevicePtr(std::string name);

  /**
   * @brief Try to add all devices
   *
   * @return int the number of devices added
   */
  int addAllDevice(bool sniff = true);

  /**
   * @brief Get MAC address of a device
   *
   * @param mac MAC address will be stored in
   * @param id the id of the device
   * @return int -1 on error
   */
  int getMACAddr(u_char *mac, int id);

  /**
   * @brief Send a frame
   *
   * @param id the device id to send
   * @param frame the frame to send
   * @return int -1 on error
   */
  int sendFrame(int id, EtherFrame &frame);

  /**
   * @brief Keep receiving packages until all the threads end
   *
   * @return int always be 0
   */
  int keepReceiving();
};

extern DeviceManager deviceMgr;
extern frameReceiveCallback callback;
}  // namespace Device

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

/**
 * @brief Keep receiving packages until all the threads end. You may use this at
 * the end of your program.
 *
 * @return int always be 0
 */
int keepReceiving();

#endif