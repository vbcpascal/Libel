#include "api.h"

#include <unordered_map>
#include <variant>

bool initialed = false;

void checkInitial() {
  if (!initialed) {
    api::init();
    initialed = true;
  }
}

namespace api {

namespace socket {
int socket(int domain, int type, int protocol) {
  checkInitial();
  return Socket::sockmgr.socket(domain, type, protocol);
}

int bind(int socket, const struct sockaddr* address, socklen_t address_len) {
  checkInitial();
  return Socket::sockmgr.bind(socket, address, address_len);
}

int listen(int socket, int backlog) {
  checkInitial();
  return Socket::sockmgr.listen(socket, backlog);
}

int connect(int socket, const struct sockaddr* address, socklen_t address_len) {
  checkInitial();
  return Socket::sockmgr.connect(socket, address, address_len);
}

int accept(int socket, struct sockaddr* address, socklen_t* address_len) {
  checkInitial();
  return Socket::sockmgr.accept(socket, address, address_len);
}

ssize_t read(int fildes, void* buf, size_t nbyte) {
  checkInitial();
  return Socket::sockmgr.read(fildes, reinterpret_cast<u_char*>(buf), nbyte);
}

ssize_t write(int fildes, const void* buf, size_t nbyte) {
  checkInitial();
  return Socket::sockmgr.write(fildes, reinterpret_cast<const u_char*>(buf),
                               nbyte);
}

ssize_t close(int fildes) {
  checkInitial();
  return Socket::sockmgr.close(fildes);
}

int getaddrinfo(const char* node, const char* service,
                const struct addrinfo* hints, struct addrinfo** res) {
  checkInitial();
  if (!node && !service) return EAI_NONAME;
  if (hints) {
    if (hints->ai_family != AF_INET) return EAI_FAMILY;
    if ((hints->ai_socktype != SOCK_STREAM) ||
        (hints->ai_protocol != IPPROTO_TCP))
      return EAI_SOCKTYPE;
    if (hints->ai_flags != 0) return EAI_BADFLAGS;
  }

  sockaddr_in* addr = new sockaddr_in;
  addr->sin_family = AF_INET;
  if (inet_aton(node, &(addr->sin_addr)) == 0) return EAI_NONAME;
  addr->sin_port = atoi(service);
  if (addr->sin_port == 0) EAI_NONAME;

  addrinfo* rp = new addrinfo;
  rp->ai_family = AF_INET;
  rp->ai_socktype = SOCK_STREAM;
  rp->ai_protocol = IPPROTO_TCP;
  rp->ai_addrlen = INET_ADDRSTRLEN;
  rp->ai_addr = reinterpret_cast<sockaddr*>(addr);
  rp->ai_next = NULL;

  *res = rp;
  return 0;
}

void freeaddrinfo(struct addrinfo* res) {
  for (auto rp = res; rp != NULL;) {
    auto nxt = rp->ai_next;
    delete rp->ai_addr;
    delete rp;
    rp = nxt;
  }
}
}  // namespace socket

std::unordered_map<u_short, commonReceiveCallback> callbackMap;

int callbackDispatcher(const void* buf, int len, DeviceId id) {
  auto frame = Ether::EtherFrame(buf, len);
  if (frame.getLength() == 0) return 0;

  frame.ntohType();
  // Printer::printEtherFrame(frame);
  auto hdr = frame.getHeader();
  auto dev = Device::deviceMgr.getDevicePtr(id);

  // MAC address: src is me?
  if (MAC::isSameMacAddr(dev->getMAC(), hdr.ether_shost)) {
    LOG_DBG("Ignore a packet, src is me! %s",
            MAC::toString(hdr.ether_shost).c_str());
    // ignore it;
    return 0;
  }

  // MAC address: dst is me or broadcast?
  if (MAC::isSameMacAddr(dev->getMAC(), hdr.ether_dhost) ||
      MAC::isBroadcast(hdr.ether_dhost)) {
    u_short type = hdr.ether_type;

    auto iter = callbackMap.find(type);

    if (iter == callbackMap.end()) {
      // LOG_ERR("Callback function not found");
      // return -1;
      return 0;
    } else {
      if (iter->second)
        return iter->second(frame.getPayload(), len - ETHER_HDR_LEN, id);
      else
        return -1;
    }
  }

  // Ignore other frame
  return 0;
}

int setCallback(u_short etherType, commonReceiveCallback callback) {
  return (callbackMap.insert_or_assign(etherType, callback)).second;
}

int init() {
  setFrameReceiveCallback(callbackDispatcher);
  setCallback(ETHERTYPE_ARP, Arp::arpCallBack);
  setCallback(ETHERTYPE_IP, Ip::ipCallBack);
  setCallback(ETHERTYPE_SDP, SDP::sdpCallBack);
  setIPPacketReceiveCallback(Socket::tcpDispatcher);
  addAllDevice(true);
  initRouter();
  return 0;
}

int sendFrame(const void* buf, int len, int ethtype, const void* destmac,
              DeviceId id) {
  return Device::deviceMgr.sendFrame(buf, len, ethtype, destmac, id);
}

int setFrameReceiveCallback(frameReceiveCallback callback) {
  // LOG_INFO("Set new callback function.");
  Device::callback = callback;
  return 0;
}

int sendIPPacket(const struct in_addr src, const struct in_addr dest, int proto,
                 const void* buf, int len) {
  return Ip::sendIPPacket(src, dest, proto, buf, len);
}

int setIPPacketReceiveCallback(IPPacketReceiveCallback callback) {
  Ip::callback = callback;
  return 0;
}

int setRoutingTable(const in_addr dest, const in_addr mask,
                    const void* nextHopMAC, const char* device) {
  auto nm = MAC::MacAddr((const u_char*)nextHopMAC);
  auto dev = Device::deviceMgr.getDevicePtr(std::string(device));
  return Route::router.addItem(
      Route::RouteItem(dest, mask, dev, nm, 1, false, SDP_METRIC_NODEL));
}

int addAllDevice(bool sniff) { return Device::deviceMgr.addAllDevice(sniff); }

void initRouter() { Route::router.init(); }
}  // namespace api