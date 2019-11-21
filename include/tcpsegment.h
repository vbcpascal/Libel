/**
 * @file tcpsegment.h
 * @author guanzhichao
 * @brief
 * @version 0.1
 * @date 2019-11-18
 *
 */

#ifndef TCPSEGMENT_H_
#define TCPSEGMENT_H_

#include <netinet/tcp.h>
#include <shared_mutex>

#include "ip.h"
#include "socketaddr.h"
#include "tcpseq.h"

namespace Tcp {

#define TCP_TYPE_MAP            \
  X(FIN, TH_FIN)                \
  X(SYN, TH_SYN)                \
  X(RST, TH_RST)                \
  X(PUSH, TH_PUSH)              \
  X(ACK, TH_ACK)                \
  X(URG, TH_URG)                \
  X(SYN_ACK, (TH_SYN + TH_ACK)) \
  X(FIN_ACK, (TH_FIN + TH_ACK))

#define X(TCPNAME, TCPFLAG) bool ISTYPE_##TCPNAME(tcphdr t);
TCP_TYPE_MAP
#undef X

#define X(TCPNAME, TCPFLAG) bool WITHTYPE_##TCPNAME(tcphdr t);
TCP_TYPE_MAP
#undef X

bool TYPE_NONE(tcphdr t);
std::string tcpFlagStr(int flags);

struct TcpSegment {
  struct __attribute__((__packed__)) {
    tcphdr hdr;
    u_char data[TCP_MAXWIN];  // with options
  };

  int totalLen;
  int dataLen;
  u_char* dataPtr;

  TcpSegment();
  TcpSegment(const u_char* buf, int len);
  TcpSegment(int sport, int dport);

  void setDefaultHdr();
  void setFlags(unsigned char flags) { hdr.th_flags = flags; }
  void setSeq(Sequence::SeqSet& ss, int len = 0);
  void setSeq(tcp_seq seq) { hdr.th_seq = seq; }
  void setAck(tcp_seq ack) { hdr.th_ack = ack; }
  void setPayload(const u_char* buf, size_t len);

  void hton();
  void ntoh();
};

struct TcpItem {
  TcpSegment ts;
  ip_addr srcIp;
  ip_addr dstIp;
  bool nonblock;

  TcpItem() : nonblock(false) {}
  TcpItem(ip_addr src, ip_addr dst, bool nonblock = false)
      : srcIp(src), dstIp(dst), nonblock(nonblock) {}
  TcpItem(const TcpSegment& ts, ip_addr src, ip_addr dst, bool nonblock = false)
      : ts(ts), srcIp(src), dstIp(dst), nonblock(nonblock) {}

  void hton();
  void ntoh();
  void setChecksum();
};

/**
 * @brief Build an ACK item automatically.
 *
 * Only one of ackLen and ackSeq can be set. ackLen will be overwrited by ackSeq
 * if both of them are set.
 *
 * @param src src address
 * @param dst dst address
 * @param ss sequence set of TcpWorker
 * @param seq_m mutex of sequence
 * @param ackLen the length to acknowledge. default: 1 for control segment
 * @param ackSeq sequence number to set in ACK
 * @return TcpItem TCP ACK item built
 */
TcpItem buildAckItem(Socket::SocketAddr src, Socket::SocketAddr dst,
                     Sequence::SeqSet& ss, std::shared_mutex& seq_m,
                     std::optional<int>(ackLen) = 1,
                     std::optional<tcp_seq>(ackSeq) = {});

}  // namespace Tcp

namespace Printer {
void printTcpItem(const Tcp::TcpItem& ts, bool sender = false,
                  std::string info = "");
}

#endif