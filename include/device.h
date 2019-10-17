/**
 * @file device.h
 * @author guanzhichao
 * @brief Library supporting network device management.
 * @version 0.1
 * @date 2019-10-02
 *
 */
#ifndef DEVICE_H_
#define DEVICE_H_

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
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
  explicit Device(std::string name, bool sniff = false);

  /**
   * @brief Get the Id object
   *
   * @return DeviceId the id
   */
  DeviceId getId();

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
   * @brief Get the Ip object
   *
   * @return ip_addr ip of device
   */
  ip_addr getIp();

  /**
   * @brief Get the Subnet Mask object
   *
   * @return ip_addr subnet mask
   */
  ip_addr getSubnetMask();

  /**
   * @brief Send a frame on the device
   *
   * @param frame the frame will be sent
   * @return int 0 on success, -1 on error
   */
  int sendFrame(Ether::EtherFrame &frame);

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
  static DeviceId max_id;
  DeviceId id;
  std::string name;
  u_char mac[ETHER_ADDR_LEN];
  ip_addr ip;
  ip_addr subnetMask;

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
  std::set<DeviceId> validId;

 public:
  /**
   * @brief Add a new device
   *
   * @param name the name of device
   * @return DeviceId id, -1 on error
   */
  DeviceId addDevice(std::string name, bool sniff = true);

  /**
   * @brief Find a device
   *
   * @param name the name of device
   * @return DeviceId id, -1 on error
   */
  DeviceId findDevice(std::string name);

  /**
   * @brief Get the pointer of device according to id
   *
   * @param id id of device
   * @return DevicePtr pointer of device
   */
  DevicePtr getDevicePtr(DeviceId id);

  /**
   * @brief Get the pointer of device according to id
   *
   * @param name name of device
   * @return DevicePtr pointer of device
   */
  DevicePtr getDevicePtr(std::string name);

  DevicePtr getDevicePtr(const ip_addr &_ip);

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
  int getMACAddr(u_char *mac, DeviceId id);

  bool haveDeviceWithIp(const ip_addr &ip);

  /**
   * @brief Send a frame
   *
   * @param dev Device pointer
   * @param frame the frame to send
   * @return int -1 on error
   */
  int sendFrame(DevicePtr dev, Ether::EtherFrame &frame);

  /**
   * @brief Send a frame
   *
   * @param id the device id to send
   * @param frame the frame to send
   * @return int -1 on error
   */
  int sendFrame(DeviceId id, Ether::EtherFrame &frame);

  /**
   * @brief
   *
   * @param buf
   * @param len
   * @param ethtype
   * @param destmac
   * @param dev
   * @return int
   */
  int sendFrame(const void *buf, int len, int ethtype, const void *destmac,
                DevicePtr dev);

  /**
   * @brief Encapsulate some data into an Ethernet II frame and send it.
   *
   * @param buf Pointer to the payload.
   * @param len Length of the payload.
   * @param ethtype EtherType field value of this frame.
   * @param destmac MAC address of the destination.
   * @param id ID of the device(returned by `addDevice`) to send on.
   * @return int 0 on success, -1 on error.
   */
  int sendFrame(const void *buf, int len, int ethtype, const void *destmac,
                DeviceId id);

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
DeviceId addDevice(const char *device);

/**
 * Find a device added by `addDevice`.
 *
 * @param device Name of the network device.
 * @return A non-negative _device-ID_ on success, -1 if no such device
 * was found.
 */
DeviceId findDevice(const char *device);

/**
 * @brief Keep receiving packages until all the threads end. You may use this at
 * the end of your program.
 *
 * @return int always be 0
 */
int keepReceiving();

#endif  // DEVICE_H_