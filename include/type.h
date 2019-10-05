/**
 * @file type.h
 * @author guanzhichao
 * @brief Define some types and constant will be used.
 * @version 0.1
 * @date 2019-10-03
 *
 */

#ifndef TYPE_H
#define TYPE_H

#include <arpa/inet.h>
#include <pcap/pcap.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "massert.h"

#ifdef __APPLE__
#include <net/if_dl.h>
#include <netinet/if_ether.h>
#define DEFAULT_DEV_NAME "en0"
#else
#include <ifaddrs.h>
#include <netinet/ether.h>
#define DEFAULT_DEV_NAME "eth0"
#endif

using DeviceId = int;

/**
 * @brief Process a frame upon receiving it.
 *
 * @param buf Pointer to the frame.
 * @param len Length of the frame.
 * @param id ID of the device (returned by `addDevice`) receiving current frame.
 * @return 0 on success, -1 on error.
 * @see addDevice
 */
using frameReceiveCallback = int (*)(const void*, DeviceId, int);

// typedef int (*frameReceiveCallback)(const void*, int, int);

#endif
