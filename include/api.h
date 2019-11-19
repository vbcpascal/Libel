/**
 * @file api.h
 * @author guanzhichao
 * @brief Library supporting api for users.
 * @version 0.1
 * @date 2019-10-18
 *
 */
#ifndef API_H_
#define API_H_

#include "arp.h"
#include "device.h"
#include "ip.h"
#include "socket.h"
#include "tcp.h"
#include "type.h"

namespace api {

namespace socket {
int socket(int domain, int type, int protocol);
int bind(int socket, const struct sockaddr *address, socklen_t address_len);
int listen(int socket, int backlog);
int connect(int socket, const struct sockaddr *address, socklen_t address_len);
int accept(int socket, struct sockaddr *address, socklen_t *address_len);
ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
ssize_t close(int fildes);
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
}  // namespace socket

/**
 * @brief Set the Callback with specific type
 *
 * @param etherType type of ether frame
 * @param callback callback function
 * @return int 1 on insert, 0 on assign, -1 on error
 */
int setCallback(u_short etherType, commonReceiveCallback callback);

/**
 * @brief Initial dispatcher and callback function
 *
 * @return int 0 on success, -1 on error
 */
int init();

/**
 * @brief Encapsulate some data into an Ethernet II frame and send it.
 *
 * @param buf Pointer to the payload.
 * @param len Length of the payload.
 * @param ethtype EtherType field value of this frame.
 * @param destmac MAC address of the destination.
 * @param id ID of the device(returned by `addDevice`) to send on.
 * @return int 0 on success, -1 on error.
 * @see addDevice
 */
int sendFrame(const void *buf, int len, int ethtype, const void *destmac,
              DeviceId id);

/**
 * @brief Register a callback function to be called each time an Ethernet II
 * frame was received.
 *
 * @param callback the callback function.
 * @return 0 on success, -1 on error.
 * @see frameReceiveCallback
 */
int setFrameReceiveCallback(frameReceiveCallback callback);

/**
 * @brief Send an IP packet to specified host.
 *
 * @param src Source IP address.
 * @param dest Destination IP address.
 * @param proto Value of `protocol` field in IP header.
 * @param buf pointer to IP payload
 * @param len Length of IP payload
 * @return 0 on success, -1 on error.
 */
int sendIPPacket(const struct in_addr src, const struct in_addr dest, int proto,
                 const void *buf, int len);

/**
 * @brief Register a callback function to be called each time an IP packet
 * was received.
 *
 * @param callback The callback function.
 * @return 0 on success, -1 on error.
 * @see IPPacketReceiveCallback
 */
int setIPPacketReceiveCallback(IPPacketReceiveCallback callback);

/**
 * @brief Manully add an item to routing table. Useful when talking with real
 * Linux machines.
 *
 * @param dest The destination IP prefix.
 * @param mask The subnet mask of the destination IP prefix.
 * @param nextHopMAC MAC address of the next hop.
 * @param device Name of device to send packets on.
 * @return 0 on success, -1 on error
 */
int setRoutingTable(const in_addr dest, const in_addr mask,
                    const void *nextHopMAC, const char *device);

/**
 * @brief Add all device of host
 *
 * @param sniff Start sniffing after open devices
 * @return int number of devices added
 */
int addAllDevice(bool sniff = false);

/**
 * @brief Initial router
 *
 */
void initRouter();
}  // namespace api

#endif