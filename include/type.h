/**
 * @file type.h
 * @author guanzhichao
 * @brief Define some types and constant will be used.
 * @version 0.1
 * @date 2019-10-03
 *
 */

#ifndef TYPE_H_
#define TYPE_H_

#include <arpa/inet.h>
#include <pcap/pcap.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "massert.h"

#ifdef __APPLE__
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <netinet/if_ether.h>
#define DEFAULT_DEV_NAME "en0"
#else
#include <ifaddrs.h>
#include <netinet/ether.h>
#define DEFAULT_DEV_NAME "eth0"
#endif

#define ETHERTYPE_SDP 0x2333

using DeviceId = int;
using ip_addr = in_addr;

/**
 * @brief Process a frame upon receiving it.
 *
 * @param buf Pointer to the frame.
 * @param len Length of the frame.
 * @param id ID of the device (returned by `addDevice`) receiving current frame.
 * @return 0 on success, -1 on error.
 * @see addDevice
 */
using frameReceiveCallback = int (*)(const void*, int, DeviceId);

/**
 * @brief Process an IP packet upon receiving it.
 *
 * @param buf Pointer to the packet.
 * @param len Length of the packet.
 * @return 0 on success, -1 on error.
 * @see addDevice
 */
using IPPacketReceiveCallback = int (*)(const void*, int);

using commonReceiveCallback = int (*)(const void*, int, DeviceId);

bool operator<(ip_addr a, ip_addr b);
bool operator==(ip_addr a, ip_addr b);

#endif  // TYPE_H_
