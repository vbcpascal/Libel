#include "socket.h"

namespace Socket {

Socket::Socket(int domain, int type, int protocol, int fd)
    : fd(fd), domain(domain), type(type), protocol(protocol) {}

int Socket::bind(const sockaddr* address, socklen_t address_len) {
  std::lock_guard<std::mutex> lock(mu);
  if (tcpWorker.getSt() == Tcp::TcpState::INVAL) RET_SETERRNO(EINVAL);
  if (!src.isZero()) RET_SETERRNO(EINVAL);
  src.set(address);
  return 0;
}

int Socket::listen(int bl) {
  std::lock_guard<std::mutex> lock(mu);
  if (tcpWorker.getSt() != Tcp::TcpState::CLOSED) RET_SETERRNO(EINVAL);
  if (src.isZero()) RET_SETERRNO(EDESTADDRREQ);
  tcpWorker.backlog = bl < 0 ? 0 : bl;
  tcpWorker.setSt(Tcp::TcpState::LISTEN);
  return 0;
}

int Socket::accept(sockaddr* address, socklen_t* address_len) {
  std::unique_lock<std::mutex> lock(tcpWorker.acceptCv_m, std::defer_lock);
  if (tcpWorker.st.load() != Tcp::TcpState::LISTEN) RET_SETERRNO(EPROTO);

  lock.lock();
  tcpWorker.acceptCv.wait(lock, [&] { return tcpWorker.pendings.size() > 0; });
  lock.unlock();

  // LISTEN --[rcv SYN, snd SYN/ACK]--> SYN_RCVD
  auto saddr_seq = tcpWorker.pendings.front();
  int fd = sockmgr.socket(this->domain, this->type, this->protocol);
  auto sock = sockmgr.getSocket(fd);
  sock->src = src;
  sock->dst = saddr_seq.first;
  sock->tcpWorker.seq.initRcvIsn(saddr_seq.second);
  sock->tcpWorker.syned.store(true);
  Printer::printSocket(sock);

  Tcp::TcpSegment ts(sock->src.port, sock->dst.port);
  ts.setFlags(TH_SYN + TH_ACK);
  ts.setSeq(sock->tcpWorker.seq, 1);  // SYN make snd_nxt +1
  ts.setAck(sock->tcpWorker.seq.sndAckWithLen(1));
  Tcp::TcpItem ti(ts, sock->src.ip, sock->dst.ip);
  sock->tcpWorker.setSt(Tcp::TcpState::SYN_RECEIVED);

  if (sock->send(ti) < 0) {
    // failed. TODO
    RET_SETERRNO(ECONNABORTED);
  }

  // SYN_RCVD --[rcv ACK]--> ESTAB
  if (sock->tcpWorker.getCriticalSt() == Tcp::TcpState::ESTABLISHED) {
    sock->tcpWorker.setSt(Tcp::TcpState::ESTABLISHED);
  }

  memcpy(address, &saddr_seq, sizeof(sockaddr));
  *address_len = INET_ADDRSTRLEN;
  return fd;
}

int Socket::connect(const sockaddr* address, socklen_t address_len) {
  std::unique_lock<std::mutex> lock(mu);
  SocketAddr dstSaddr(address);
  dst = dstSaddr;

  // set src ip and port (if not bind)
  auto addr_in = dst.ip;
  if (src.ip.s_addr == 0) {
    Device::DevicePtr dev = Route::router.lookup(addr_in).dev;
    src.ip = dev->getIp();
  }
  if (src.port == 0) {
    if (sockmgr.nextPort.find(addr_in) != sockmgr.nextPort.end()) {
      src.port = (sockmgr.nextPort[addr_in]++);
    } else {
      src.port = 2048;
      sockmgr.nextPort[addr_in] = 2049;
    }
  }

  LOG_INFO("Socket %s:%d try to connect %s.", Ip::ipToStr(src.ip).c_str(),
           src.port, dst.toStr().c_str());

  // send SYN
  Tcp::TcpSegment ts(src.port, dst.port);

  {  // CLOSED --[send SYN]--> SYN_SENT
    ts.setFlags(TH_SYN);
    ts.setSeq(tcpWorker.seq, 1);
    Tcp::TcpItem ti(ts, src.ip, dst.ip);
    tcpWorker.setSt(Tcp::TcpState::SYN_SENT);

    if (this->send(ti) < 0) {
      // TODO: connect failed
      RET_SETERRNO(ETIMEDOUT);
    }
  }

  // ^ STN_SENT --[rcv SYN/ACK, snd ACK]--> ESTAB
  if (tcpWorker.getCriticalSt() == Tcp::TcpState::ESTABLISHED) {
    auto ti = Tcp::buildAckItem(src, dst, tcpWorker.seq, tcpWorker.seq_m);
    Printer::printTcpItem(ti);
    tcpWorker.setSt(Tcp::TcpState::ESTABLISHED);
    this->send(ti);
    return 0;
  }

  // ^ SYN_SENT --[rcv SYN, snd SYN/ACK]--> SYN_RCVD
  if (tcpWorker.getCriticalSt() == Tcp::TcpState::SYN_RECEIVED) {
    ts.setFlags(TH_ACK + TH_SYN);
    ts.setSeq(tcpWorker.seq, 1);
    ts.setAck(tcpWorker.seq.sndAckWithLen(1));
    Tcp::TcpItem ti(ts, addr_in, dst.ip);
    tcpWorker.setSt(Tcp::TcpState::SYN_RECEIVED);

    if (this->send(ti) < 0) {
      // TODO: connect failed
      RET_SETERRNO(ETIMEDOUT);
    }
  }

  // ^ SYN_RCVD --[rcv ACK]--> ESTAB
  if (tcpWorker.getCriticalSt() == Tcp::TcpState::ESTABLISHED) {
    tcpWorker.setSt(Tcp::TcpState::ESTABLISHED);
    return 0;
  }

  return 0;
}

ssize_t Socket::read(u_char* buf, size_t nbyte) {
  return tcpWorker.read(buf, nbyte);
}

ssize_t Socket::write(const u_char* buf, size_t nbyte) {
  tcp_seq s = tcpWorker.seq.allocateWithLen(nbyte);
  Tcp::TcpSegment ts(src.port, dst.port);
  ts.setPayload(buf, nbyte);
  ts.setSeq(s);
  Tcp::TcpItem ti(ts, src.ip, dst.ip);

  return tcpWorker.send(ti);
}

ssize_t Socket::send(const Tcp::TcpItem& ti) { return tcpWorker.send(ti); }

int Socket::close() {
  // SYN_RCVD --[CLOSE, snd FIN]--> FIN_WAIT_1
  if (tcpWorker.getSt() == Tcp::TcpState::LISTEN) {
    Tcp::TcpSegment ts(src.port, dst.port);
    ts.setFlags(TH_FIN);
    ts.setSeq(tcpWorker.seq, 1);
    ts.setAck(tcpWorker.seq.rcv_nxt);
    Tcp::TcpItem ti(ts, src.ip, dst.ip);
    tcpWorker.setSt(Tcp::TcpState::FIN_WAIT_1);

    if (this->send(ti) < 0) {
      // TODO
    }
  }

  // ESTAB --[CLOSE, snd FIN]--> FIN_WAIT_1
  if (tcpWorker.getSt() == Tcp::TcpState::ESTABLISHED) {
    Tcp::TcpSegment ts(src.port, dst.port);
    ts.setFlags(TH_FIN);
    ts.setSeq(tcpWorker.seq, 1);
    ts.setAck(tcpWorker.seq.rcv_nxt);
    Tcp::TcpItem ti(ts, src.ip, dst.ip);
    tcpWorker.setSt(Tcp::TcpState::FIN_WAIT_1);

    if (this->send(ti) < 0) {
      // TODO
    }
  }

  // ^ FIN_WAIT_1 --[rcv FIN/ACK, snd ACK]--> TIME_WAIT
  if (tcpWorker.getSt() == Tcp::TcpState::FIN_WAIT_1 &&
      tcpWorker.getCriticalSt() == Tcp::TcpState::TIMED_WAIT) {
    auto ti = Tcp::buildAckItem(src, dst, tcpWorker.seq, tcpWorker.seq_m);
    tcpWorker.setSt(Tcp::TcpState::TIMED_WAIT);
    this->send(ti);
  }

  // ^ FIN_WAIT_1 --[rcv ACK]--> FIN_WAIT_2
  if (tcpWorker.getSt() == Tcp::TcpState::FIN_WAIT_1 &&
      tcpWorker.getCriticalSt() == Tcp::TcpState::FIN_WAIT_2) {
    tcpWorker.setSt(Tcp::TcpState::FIN_WAIT_2);
  }

  // ^ FIN_WAIT_2 --[rcv FIN, snd ACK]--> TIME_WAIT
  if (tcpWorker.getSt() == Tcp::TcpState::FIN_WAIT_2 &&
      tcpWorker.getCriticalSt() == Tcp::TcpState::TIMED_WAIT) {
    auto ti = Tcp::buildAckItem(src, dst, tcpWorker.seq, tcpWorker.seq_m);
    tcpWorker.setSt(Tcp::TcpState::TIMED_WAIT);
    this->send(ti);
  }

  // ^ FIN_WAIT_1 --[rcv FIN, snd ACK]--> CLOSING
  if (tcpWorker.getSt() == Tcp::TcpState::FIN_WAIT_1 &&
      tcpWorker.getCriticalSt() == Tcp::TcpState::CLOSING) {
    auto ti = Tcp::buildAckItem(src, dst, tcpWorker.seq, tcpWorker.seq_m);
    tcpWorker.setSt(Tcp::TcpState::TIMED_WAIT);
    this->send(ti);
  }

  // ^ CLOSING --[rcv ACK]--> TIME_WAIT
  if (tcpWorker.getSt() == Tcp::TcpState::CLOSING &&
      tcpWorker.getCriticalSt() == Tcp::TcpState::TIMED_WAIT) {
    tcpWorker.setSt(Tcp::TcpState::TIMED_WAIT);
  }

  // ^ TIMED_WAIT --[Timeout]--> CLOSED
  if (tcpWorker.getSt() == Tcp::TcpState::TIMED_WAIT) {
    std::this_thread::sleep_for(std::chrono::seconds(MSL * 2));
    tcpWorker.setSt(Tcp::TcpState::CLOSED);
  }

  // CLOSE_WAIT --[CLOSE, snd FIN]--> WAIT
  if (tcpWorker.getSt() == Tcp::TcpState::CLOSE_WAIT) {
    Tcp::TcpSegment ts(src.port, dst.port);
    ts.setFlags(TH_FIN);
    ts.setSeq(tcpWorker.seq, 1);
    ts.setAck(tcpWorker.seq.rcv_nxt);
    Tcp::TcpItem ti(ts, src.ip, dst.ip);
    tcpWorker.setSt(Tcp::TcpState::LAST_ACK);

    if (this->send(ti) < 0) {
      // todo
    }
  }

  // ^ LAST_ACK --[rcv ACK]--> CLOSED
  if (tcpWorker.getSt() == Tcp::TcpState::LAST_ACK &&
      tcpWorker.getCriticalSt() == Tcp::TcpState::CLOSED) {
    tcpWorker.setSt(Tcp::TcpState::CLOSED);
  }

  return 0;
}

//================= SocketManager =================//

SocketPtr SocketManager::getSocket(int fd) {
  for (auto& i : socketList) {
    if (i->fd == fd) return i;
  }
  return nullptr;
}

SocketPtr SocketManager::getSocket(const SocketAddr src, const SocketAddr dst) {
  for (auto& i : socketList) {
    if (i->src == src && i->dst == dst) return i;
  }
  return nullptr;
}

SocketPtr SocketManager::getListeningSocket(const SocketAddr src) {
  for (auto& i : socketList) {
    if (i->src == src && i->tcpWorker.getSt() == Tcp::TcpState::LISTEN)
      return i;
  }
  return nullptr;
}

int SocketManager::socket(int domain, int type, int protocol) {
  if (nextFd == -1) RET_SETERRNO(ENFILE);
  if (domain != AF_INET) RET_SETERRNO(EAFNOSUPPORT);
  if (type != SOCK_STREAM) RET_SETERRNO(EPROTOTYPE);
  if (protocol != IPPROTO_TCP) RET_SETERRNO(EPROTONOSUPPORT);
  auto sock = std::make_shared<Socket>(domain, type, protocol, nextFd++);
  socketList.push_back(sock);
  return sock->fd;
}

int SocketManager::bind(int socket, const struct sockaddr* address,
                        socklen_t address_len) {
  auto addr_in = reinterpret_cast<const struct sockaddr_in*>(address);
  if (!addr_in) RET_SETERRNO(EADDRNOTAVAIL);
  if (addr_in->sin_family != AF_INET) RET_SETERRNO(EAFNOSUPPORT);
#ifdef __APPLE__
  if (addr_in->sin_len != INET_ADDRSTRLEN) RET_SETERRNO(EINVAL);
#endif
  // if (getSocket(address)) RET_SETERRNO(EADDRINUSE);
  SocketPtr s = getSocket(socket);
  if (!s) RET_SETERRNO(EBADF);
  return s->bind(address, address_len);
}

int SocketManager::listen(int socket, int backlog) {
  SocketPtr s = getSocket(socket);
  if (!s) RET_SETERRNO(EBADF);
  return s->listen(backlog);
}

int SocketManager::accept(int socket, sockaddr* address,
                          socklen_t* address_len) {
  SocketPtr s = getSocket(socket);
  if (!s) RET_SETERRNO(EBADF);
  return s->accept(address, address_len);
}

int SocketManager::connect(int socket, const sockaddr* address,
                           socklen_t address_len) {
  SocketPtr s = getSocket(socket);
  if (!s) RET_SETERRNO(EBADF);
  return s->connect(address, address_len);
}

ssize_t SocketManager::read(int fildes, u_char* buf, size_t nbyte) {
  SocketPtr s = getSocket(fildes);
  if (!s) RET_SETERRNO(EBADF);
  return s->read(buf, nbyte);
}
ssize_t SocketManager::write(int fildes, const u_char* buf, size_t nbyte) {
  SocketPtr s = getSocket(fildes);
  if (!s) RET_SETERRNO(EBADF);
  return s->write(buf, nbyte);
}

int SocketManager::close(int fildes) {
  SocketPtr s = getSocket(fildes);
  if (!s) RET_SETERRNO(EBADF);
  return s->close();
}

SocketManager sockmgr;

int tcpDispatcher(const void* buf, int len) {
  Ip::IpPacket ipp((u_char*)buf, len);
  SocketAddr srcSaddr, dstSaddr;
  srcSaddr.ip = ipp.hdr.ip_src;
  dstSaddr.ip = ipp.hdr.ip_dst;
  LOG_ERR("%d %d", len, ipp.hdr.ip_hl);
  Tcp::TcpSegment ts((u_char*)ipp.data, len - ipp.hdr.ip_hl * 4);
  srcSaddr.port = ts.hdr.th_sport;
  dstSaddr.port = ts.hdr.th_dport;
  Tcp::TcpItem ti(ts, ipp.hdr.ip_src, ipp.hdr.ip_dst);
  // Printer::printIpPacket(ipp);
  Printer::printTcpItem(ti);

  SocketPtr sock;
  if (Tcp::ISTYPE_SYN(ts.hdr)) {
    sock = sockmgr.getListeningSocket(dstSaddr);
  } else {
    sock = sockmgr.getSocket(dstSaddr, srcSaddr);
  }
  if (!sock) {
    LOG_WARN("A segment cannot match any local socket.");
    return 0;
  }
  sock->tcpWorker.handler(ti);
  return 0;
}
}  // namespace Socket

namespace Printer {
void printSocket(const Socket::SocketPtr& sock) {
  printf("Socket: %d, src=%s, dst=%s, st=%s\n", sock->fd,
         sock->src.toStr().c_str(), sock->dst.toStr().c_str(),
         Tcp::stateToStr(sock->tcpWorker.getSt()).c_str());
}
}  // namespace Printer