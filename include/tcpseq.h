/**
 * @file tcpseq.h
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-11-16
 *
 * @brief TCP sequence number.
 *
 */

#ifndef TCPSEQ_H_
#define TCPSEQ_H_

#define SEQ_ZERO

#include <netinet/tcp.h>

#include <atomic>
#include <thread>

namespace Tcp::Sequence {

/**
 * @brief To generate initial sequence number
 *
 */
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

/**
 * @brief Sequence set in a communication
 *
 */
struct SeqSet {
  /**
   * @brief sender initial sequence number
   *
   */
  tcp_seq snd_isn;

  /**
   * @brief oldest unacknowledged seq num
   *
   */
  tcp_seq snd_una;

  /**
   * @brief next seq num to be sent
   *
   */
  tcp_seq snd_nxt;

  /**
   * @brief receive initial sequence number
   *
   */
  tcp_seq rcv_isn;

  /**
   * @brief next seq num expected on an incoming segment
   *
   */
  tcp_seq rcv_nxt;

  /**
   * @brief Initial `rcv_isn` and `rcv_nxt`
   *
   */
  void initRcvIsn(tcp_seq s);

  /**
   * @brief Allocate sequence number for a segment with length `len`
   *
   */
  tcp_seq allocateWithLen(int len);

  /**
   * @brief Update `snd_una` for an ACK with length `len`
   *
   */
  tcp_seq rcvAckWithLen(int len);

  /**
   * @brief Get the ACK number when sending an ACK with length `len`
   *
   */
  tcp_seq sndAckWithLen(int len);

  /**
   * @brief Compare and update sequence number
   *
   */
  bool tryAndRcvAck(tcphdr hdr);  // no sliding window only

  /**
   * @brief Convert the sequence set to a string
   *
   */
  std::string toStr() const;
};

bool lessThan(tcp_seq lhs, tcp_seq rhs, tcp_seq base);
bool greaterThan(tcp_seq lhs, tcp_seq rhs, tcp_seq base);
bool equalTo(tcp_seq lhs, tcp_seq rhs);

}  // namespace Tcp::Sequence

namespace Printer {
/**
 * @brief Print a sequence number set
 *
 * @param ss sequence number set
 */
void printSeq(const Tcp::Sequence::SeqSet& ss);
}  // namespace Printer

#endif
