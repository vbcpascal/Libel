/**
 * @file sequence.h
 * @author guanzhichao
 * @brief
 * @version 0.1
 * @date 2019-11-17
 *
 */

#ifndef TCPSEQ_H_
#define TCPSEQ_H_

#define SEQ_ZERO

#include <netinet/tcp.h>

#include <atomic>
#include <thread>

namespace Tcp::Sequence {

// To generate initial sequence number
class ISNGenerator {
 private:
  std::atomic<tcp_seq> isn;
  std::thread setter;

 public:
  ISNGenerator();
  tcp_seq getISN();
  void setISN();
};

extern ISNGenerator isnGen;

struct SeqSet {
  tcp_seq snd_isn;  // sender initial sequence number
  tcp_seq snd_una;  // oldest unacknowledged seq num
  tcp_seq snd_nxt;  // next seq num to be sent
  tcp_seq rcv_isn;  // receive initial sequence number
  tcp_seq rcv_nxt;  // next seq num expected on an incoming segment

  void initRcvIsn(tcp_seq s);
  tcp_seq allocateWithLen(int len);
  tcp_seq rcvAckWithLen(int len);
  tcp_seq sndAckWithLen(int len);
  bool tryAndRcvAck(tcphdr hdr);  // no sliding window only
  std::string toStr() const;
};

// last ACK == rcv_nxt

bool lessThan(tcp_seq lhs, tcp_seq rhs, tcp_seq base);
bool greaterThan(tcp_seq lhs, tcp_seq rhs, tcp_seq base);
bool equalTo(tcp_seq lhs, tcp_seq rhs);

}  // namespace Tcp::Sequence

namespace Printer {
void printSeq(const Tcp::Sequence::SeqSet& ss);
}

#endif
