#include "tcpseq.h"

namespace Tcp::Sequence {

ISNGenerator isnGen;

ISNGenerator::ISNGenerator() : isn(0) {
  setter = std::thread([&] { setISN(); });
}

tcp_seq ISNGenerator::getISN() {
  std::lock_guard<std::mutex> lock(isn_m);
  return isn;
}

void ISNGenerator::setISN() {
  std::unique_lock<std::mutex> lock(isn_m, std::defer_lock);

  while (true) {
    std::this_thread::sleep_for(std::chrono::microseconds(4));
    lock.lock();
    ++isn;  // from 0 to 2^32 - 1
    lock.unlock();
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
