/**
 * @file type.h
 * @author guanzhichao
 * @brief
 * @version 0.1
 * @date 2019-10-03
 *
 * @copyright Copyright (c) 2019
 *
 */

#ifndef TYPE_H
#define TYPE_H

#include <pcap/pcap.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "massert.h"

#ifdef __APPLE__
#include <netinet/if_ether.h>
#else
#include <netinet/ether.h>
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