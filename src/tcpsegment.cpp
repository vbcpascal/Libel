#include "tcpsegment.h"

namespace {
struct TcpWithPsd {
  struct __attribute__((__packed__)) {
    ip_addr src;
    ip_addr dst;
    uint8_t zeros;
    uint8_t proto;
    uint16_t tcp_len;
    u_char tcp_data[TCP_MAXWIN];
  } _;
  uint16_t total_len;

  TcpWithPsd(const Tcp::TcpItem& ti) {
    total_len = ti.ts.totalLen + 12;
    _.src = ti.srcIp;
    _.dst = ti.dstIp;
    _.zeros = 0;
    _.proto = IPPROTO_TCP;
    _.tcp_len = htons(ti.ts.totalLen);
    memcpy(&_.tcp_data, &ti.ts.hdr, ti.ts.totalLen);
  }

  uint16_t getChecksum() { return Ip::getChecksum(&_, total_len); }

  void print() {
    printf("TCP:: 0x%x, TotalLen:%d -> checksum: %d\n", _.proto, total_len,
           getChecksum());
  }
};
}  // namespace

namespace Tcp {

std::string tcpFlagStr(int flags) {
  std::string s;
#define X(TCPNAME, TCPFLAG) \
  if (flags & TCPFLAG) {    \
    flags -= TCPFLAG;       \
    if (s != "") s += "/";  \
    s += #TCPNAME;          \
  }
  TCP_TYPE_MAP
#undef X

  if (s == "") s = "(Data)";
  return s;
}

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
  hdr.th_sum = 0;
}

void TcpSegment::ntoh() {
  hdr.th_sport = ntohs(hdr.th_sport);
  hdr.th_dport = ntohs(hdr.th_dport);
  hdr.th_seq = ntohl(hdr.th_seq);
  hdr.th_ack = ntohl(hdr.th_ack);
  hdr.th_win = ntohs(hdr.th_win);
  hdr.th_urp = ntohs(hdr.th_urp);
  hdr.th_sum = 0;
}

void TcpItem::hton() { ts.hton(); }
void TcpItem::ntoh() { ts.ntoh(); }
void TcpItem::setChecksum() {
  TcpWithPsd twp(*this);
  ts.hdr.th_sum = twp.getChecksum();
}

TcpItem buildAckItem(Socket::SocketAddr src, Socket::SocketAddr dst,
                     Sequence::SeqSet& ss, std::shared_mutex& seq_m,
                     std::optional<int>(ackLen),
                     std::optional<tcp_seq>(ackSeq)) {
  std::lock_guard lock(seq_m);
  TcpItem ti(src.ip, dst.ip, true);
  ti.ts.setFlags(TH_ACK);
  ti.ts.setSeq(ss, 0);
  ti.ts.hdr.th_sport = src.port;
  ti.ts.hdr.th_dport = dst.port;
  if (ackSeq.has_value()) {
    ti.ts.setAck(ackSeq.value());
  } else if (ackLen.has_value()) {
    ti.ts.setAck(ss.sndAckWithLen(ackLen.value()));
  } else {
    LOG_ERR("Bad ACK item");
  }

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
void printTcpItem(const Tcp::TcpItem& ti, bool sender, std::string info) {
  Socket::SocketAddr srcSaddr(ti.srcIp, ti.ts.hdr.th_sport);
  Socket::SocketAddr dstSaddr(ti.dstIp, ti.ts.hdr.th_dport);
  printf("%s \033[;1mTCP\033[0m %s -> %s, f=0x%02x %s\t %s\n",
         (sender ? "--" : ">>"), srcSaddr.toStr().c_str(),
         dstSaddr.toStr().c_str(), ti.ts.hdr.th_flags,
         Tcp::tcpFlagStr(ti.ts.hdr.th_flags).c_str(), info.c_str());
  printf("   [ seq=%u, ack=%u ], len=%d(%d)\n", ti.ts.hdr.th_seq,
         ti.ts.hdr.th_ack, ti.ts.dataLen, ti.ts.totalLen);
}
}  // namespace Printer