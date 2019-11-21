#include "tcpseq.h"

namespace Tcp::Sequence {

ISNGenerator isnGen;

ISNGenerator::ISNGenerator() : isn(0) {
#ifndef SEQ_ZERO
  setter = std::thread([&] { setISN(); });
  setter.detach();
#endif
}

tcp_seq ISNGenerator::getISN() {
#ifdef SEQ_ZERO
  return 0;
#else
  return isn.load();
#endif
}

void ISNGenerator::setISN() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::microseconds(4));
    isn.store(isn.load() + 1);  // from 0 to 2^32 - 1
  }
}

//=================== SeqSet ===================//

void SeqSet::initRcvIsn(tcp_seq s) {
  rcv_isn = s;
  rcv_nxt = s;
}

tcp_seq SeqSet::rcvAckWithLen(int len) {
  snd_una += len;
  return snd_una;
}

tcp_seq SeqSet::allocateWithLen(int len) {
  tcp_seq s = snd_nxt;
  snd_nxt += len;
  return s;
}

tcp_seq SeqSet::sndAckWithLen(int len) {
  rcv_nxt += len;
  return rcv_nxt;
}

bool SeqSet::tryAndRcvAck(tcphdr hdr) {
  if (equalTo(snd_nxt, hdr.th_ack)) {
    rcvAckWithLen(snd_nxt - snd_una);
    return true;
  } else
    return false;
}

std::string SeqSet::toStr() const {
  char buf[100];
  sprintf(buf, "%d->%d->%d, %d->%d", snd_isn, snd_una, snd_nxt, rcv_isn,
          rcv_nxt);
  return std::string(buf);
}

bool lessThan(tcp_seq lhs, tcp_seq rhs, tcp_seq base) {
  lhs = (lhs >= base ? lhs - base : lhs + (sizeof(tcp_seq) - base));
  rhs = (rhs >= base ? rhs - base : rhs + (sizeof(tcp_seq) - base));
  return (lhs < rhs);
}

bool greaterThan(tcp_seq lhs, tcp_seq rhs, tcp_seq base) {
  lhs = (lhs >= base ? lhs - base : lhs + (sizeof(tcp_seq) - base));
  rhs = (rhs >= base ? rhs - base : rhs + (sizeof(tcp_seq) - base));
  return (lhs > rhs);
}

bool equalTo(tcp_seq lhs, tcp_seq rhs) { return (lhs == rhs); }
}  // namespace Tcp::Sequence

namespace Printer {
void printSeq(const Tcp::Sequence::SeqSet& ss) {
  printf("\033[;1mSeq:\033[0m %s\n", ss.toStr().c_str());
}
}  // namespace Printer