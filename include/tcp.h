/**
 * @file tcp.h
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-10-24
 *
 * @brief Main TCP functions and status.
 *
 */

#ifndef TCP_H_
#define TCP_H_

#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <atomic>
#include <list>
#include <queue>
#include <shared_mutex>

#include "ip.h"
#include "socketaddr.h"
#include "tcpbuffer.h"
#include "tcpsegment.h"
#include "tcpseq.h"
#include "tcpstate.h"

#define SETERRNO(x) errno = x
#define RET_SETERRNO(x) \
  {                     \
    SETERRNO(x);        \
    return -1;          \
  }

namespace Tcp {
extern Sequence::ISNGenerator isnGen;

constexpr int tcpTimeout = 3;
constexpr int tcpMaxRetrans = 2;

class TcpWorker {
 public:
  bool closed = false;

  std::atomic_bool syned;
  std::atomic<TcpState> st;          // Current state of TCP connection
  std::atomic<TcpState> criticalSt;  // Critical State, for DFA
  std::condition_variable stSameCv;  // they are the same!
  std::condition_variable stCriticalChangeCv;  // Critical Stage changed
  std::mutex stSameCv_m;
  std::mutex stCCCv_m;

  Sequence::SeqSet seq;               // Sequence number
  std::shared_mutex seq_m;            // Mutex of sequence number
  std::condition_variable_any seqCv;  // wait for rcv_nxt > currSeq
  std::set<tcp_seq> abandonedSeq;     // Abandoned sequence number
  std::mutex abanseq_m;

  std::queue<TcpItem> sendList;          // Queue of segment to send
  std::queue<TcpItem> sendNonBlockList;  // Queue of segment to send
  BufferQueue recvBuf;                   // List of segment received
  std::shared_mutex sendlst_m;           // Mutex of sendList
  std::shared_mutex sendNonBlocklst_m;   // Mutex of sendList
  std::shared_mutex recvbuf_m;           // Mutex of recvBuffer
  std::thread sender;
  std::thread senderNonBlock;
  std::condition_variable_any sendCv;          // wait for not empty
  std::condition_variable_any sendNonBlockCv;  // wait for not empty
  std::condition_variable_any recvCv;          // wait for not empty

  int backlog;  // ength of the listen queue. 0 for any
  std::queue<std::pair<Socket::SocketAddr, tcp_seq>>
      pendings;                      // pending connections
  std::condition_variable acceptCv;  // cv for socket::Accept
  std::mutex acceptCv_m;             // Mutex of accept cv

 public:
  TcpWorker();
  ~TcpWorker();
  TcpState getSt();
  void setSt(TcpState newst);
  TcpState getCriticalSt();
  void setCriticalSt(TcpState newst);

  ssize_t send(TcpItem& ti);
  ssize_t read(u_char* buf, size_t nbyte);
  void handler(TcpItem& ts);
  void senderLoop();
  void senderNonBlockLoop();
};

}  // namespace Tcp

#endif

/**
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |          Source Port          |       Destination Port        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        Sequence Number                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Acknowledgment Number                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Data |           |U|A|P|R|S|F|                               |
 * | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
 * |       |           |G|K|H|T|N|N|                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           Checksum            |         Urgent Pointer        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Options                    |    Padding    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                             data                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */