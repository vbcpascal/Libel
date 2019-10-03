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
#include <netinet/if_ether.h>
#define DEFAULT_DEV_NAME "en0"
#else
#include <netinet/ether.h>
#define DEFAULT_DEV_NAME "eth0"
#endif

/**
 * @brief Process a frame upon receiving it.
 *
 * @param buf Pointer to the frame.
 * @param len Length of the frame.
 * @param id ID of the device (returned by `addDevice`) receiving current frame.
 * @return 0 on success, -1 on error.
 * @see addDevice
 */
typedef int (*frameReceiveCallback)(const void*, int, int);

#endif