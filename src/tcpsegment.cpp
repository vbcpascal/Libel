#include "tcpsegment.h"

namespace Tcp {

TcpSegment::TcpSegment() { setDefaultHdr(); }

TcpSegment::TcpSegment(const u_char* buf, int len) {
  memset(&hdr, 0, sizeof(TcpSegment));
  memcpy(&hdr, buf, len);
  this->totalLen = len;
  this->dataLen = len - hdr.th_off * 4;
  dataPtr = reinterpret_cast<u_char*>(&hdr) + hdr.th_off * 4;
}

TcpSegment::TcpSegment(int sport, int dport) {
  setDefaultHdr();
  hdr.th_sport = sport;
  hdr.th_dport = dport;
  dataPtr = data;
}

void TcpSegment::setDefaultHdr() {
  hdr.th_sport = hdr.th_dport = 0;
  hdr.th_seq = hdr.th_ack = 0;
  hdr.th_off = sizeof(tcphdr) / 4;
  hdr.th_flags = 0;
  hdr.th_win = UINT16_MAX;
  hdr.th_urp = 0;

  dataLen = 0;
  totalLen = dataLen + 4 * hdr.th_off;
  dataPtr = data;
}

void TcpSegment::setSeq(Sequence::SeqSet& ss, int len) {
  hdr.th_seq = ss.allocateWithLen(len);
}

void TcpSegment::setPayload(const u_char* buf, size_t len) {
  std::copy_n(buf, len, data);
  dataLen = len;
  totalLen = dataLen + hdr.th_off * 4;
}

void TcpSegment::hton() {
  hdr.th_sport = htons(hdr.th_sport);
  hdr.th_dport = htons(hdr.th_dport);
  hdr.th_seq = htonl(hdr.th_seq);
  hdr.th_ack = htonl(hdr.th_ack);
  hdr.th_win = htons(hdr.th_win);
  hdr.th_urp = htons(hdr.th_urp);
}

void TcpSegment::ntoh() {
  hdr.th_sport = ntohs(hdr.th_sport);
  hdr.th_dport = ntohs(hdr.th_dport);
  hdr.th_seq = ntohl(hdr.th_seq);
  hdr.th_ack = ntohl(hdr.th_ack);
  hdr.th_win = ntohs(hdr.th_win);
  hdr.th_urp = ntohs(hdr.th_urp);
}

TcpItem buildAckItem(Socket::SocketAddr src, Socket::SocketAddr dst,
                     Sequence::SeqSet& ss, std::shared_mutex& seq_m,
                     std::optional<tcp_seq>(ackSeq)) {
  std::lock_guard lock(seq_m);
  TcpItem ti(src.ip, dst.ip, true);
  ti.ts.setFlags(TH_ACK);
  ti.ts.setSeq(ss, 0);
  if (ackSeq.has_value())
    ti.ts.setAck(ackSeq.value());
  else
    ti.ts.setAck(ss.sndAckWithLen(1));

  return ti;
}

#define X(TCPNAME, TCPFLAG) \
  bool ISTYPE_##TCPNAME(tcphdr t) { return (t.th_flags) == TCPFLAG; }
TCP_TYPE_MAP
#undef X

#define X(TCPNAME, TCPFLAG) \
  bool WITHTYPE_##TCPNAME(tcphdr t) { return (t.th_flags & TCPFLAG) != 0; }
TCP_TYPE_MAP
#undef X

bool TYPE_NONE(tcphdr t) { return t.th_flags == 0; }

}  // namespace Tcp

namespace Printer {
void printTcpItem(const Tcp::TcpItem& ti, bool sender) {
  Socket::SocketAddr srcSaddr(ti.srcIp, ti.ts.hdr.th_sport);
  Socket::SocketAddr dstSaddr(ti.dstIp, ti.ts.hdr.th_dport);
  printf("%s \033[;1mTCP\033[0m %s -> %s, f=0x%x\n", (sender ? "--" : ">>"),
         srcSaddr.toStr().c_str(), dstSaddr.toStr().c_str(),
         ti.ts.hdr.th_flags);
  printf("   [ seq=%d, ack=%d ], len=%d(%d)\n", ti.ts.hdr.th_seq,
         ti.ts.hdr.th_ack, ti.ts.dataLen, ti.ts.totalLen);
}
}  // namespace Printer