#include "tcp.h"

#define SENDLST_POP(lck) \
  {                      \
    lck.lock();          \
    sendList.pop();      \
    lck.unlock();        \
  }

namespace Tcp {

TcpWorker::TcpWorker() : st(TcpState::CLOSED) {
  seq.snd_una = seq.snd_nxt = seq.snd_isn = Sequence::isnGen.getISN();
  syned.store(false);
  sender = std::thread([&] { senderLoop(); });
  senderNonBlock = std::thread([&] { senderNonBlockLoop(); });
  sender.detach();
  senderNonBlock.detach();
}

TcpWorker::~TcpWorker() {
  closed = true;
  stSameCv.notify_all();
  stCriticalChangeCv.notify_all();
  seqCv.notify_all();
  sendCv.notify_all();
  sendNonBlockCv.notify_all();
  recvCv.notify_all();
  acceptCv.notify_all();
}

TcpState TcpWorker::getSt() {
  auto s = st.load();
  return s;
}

void TcpWorker::setSt(TcpState newst) {
  // LOG_INFO("Change State to: \033[33;1m%s\033[0m",
  // stateToStr(newst).c_str());
  st.store(newst);
  criticalSt.store(newst);
  stSameCv.notify_all();

  if (newst == TcpState::CLOSED) {
    std::lock_guard lk(sendlst_m);
    std::queue<TcpItem>{}.swap(sendList);  // clear
  }
  return;
}

TcpState TcpWorker::getCriticalSt() {
  auto s = criticalSt.load();
  return s;
}

void TcpWorker::setCriticalSt(TcpState newst) {
  // LOG_INFO("Change Critical State to: \033[33m%s\033[0m",
  //          stateToStr(newst).c_str());
  criticalSt.store(newst);
  stCriticalChangeCv.notify_all();
  return;
}

ssize_t TcpWorker::send(TcpItem& ti) {
  /*
   * Should be the UNIQUE entrance to send a packet. Similiar to senderLoop
   * mentioned next, we will compare the sequence number to send and rcv_nxt.
   * After that, if the abandonedSeq has the same seq, it failed.
   *
   * See also: senderLoop
   */
  auto currSeq = ti.ts.hdr.th_seq;

  if (ti.nonblock) {
    std::unique_lock<std::shared_mutex> lock(sendNonBlocklst_m);
    sendNonBlockList.push(ti);
    lock.unlock();
    sendNonBlockCv.notify_all();
  } else {  // push the segment to sendList
    std::unique_lock<std::shared_mutex> lock(sendlst_m);
    sendList.push(ti);
    lock.unlock();
    sendCv.notify_all();
  }

  if (ti.nonblock) return 0;

  {  // wait for sender
    std::shared_lock<std::shared_mutex> lock(seq_m);
    seqCv.wait(lock, [&]() {
      return Sequence::greaterThan(seq.snd_una, currSeq, seq.snd_isn);
    });
    lock.unlock();

    std::unique_lock<std::mutex> aslock(abanseq_m);
    if (abandonedSeq.find(currSeq) != abandonedSeq.end()) {
      abandonedSeq.erase(currSeq);
      RET_SETERRNO(ECONNRESET);
    }
    aslock.unlock();
  }
  return ti.ts.dataLen;
}

void TcpWorker::senderLoop() {
  /*
   * Send segment in sendList. If the top of queue with a different sequence
   * number from current number, it means that last segment has be sent
   * successfully. And the current sequence will be set to 0. Otherwise, sender
   * will try to send the segment again. abandonedSeq is used in `write`.
   *
   * If the currSeq is 0, the sender will get a new segment from list (without
   * poping). So it's not a problem though the sequence number is 0 itself.
   *
   * See more: send
   */
  std::shared_lock<std::shared_mutex> seqLock(seq_m, std::defer_lock);
  std::unique_lock<std::shared_mutex> sendLock(sendlst_m, std::defer_lock);
  std::unique_lock<std::mutex> abanseqLock(abanseq_m, std::defer_lock);
  TcpItem ti;
  int retransCnt = tcpMaxRetrans;
  tcp_seq currSeq = 0;

  while (true) {
    // wait for something if no sengment
    if (currSeq == 0) {
      sendLock.lock();
      sendCv.wait(sendLock, [&] { return (closed || sendList.size() > 0); });
      if (closed) return;
      ti = sendList.front();
      sendLock.unlock();
      Printer::printTcpItem(ti, true);
      currSeq = ti.ts.hdr.th_seq;
      ti.ntoh();
      ti.setChecksum();
    }
    Ip::sendIPPacket(ti.srcIp, ti.dstIp, IPPROTO_TCP, &ti.ts, ti.ts.totalLen);

    if (ti.nonblock) {
      // send next segment if nonblock
      retransCnt = tcpMaxRetrans;
      currSeq = 0;
      SENDLST_POP(sendLock);
      continue;
    }

    seqLock.lock();
    if (seqCv.wait_for(seqLock, std::chrono::seconds(tcpTimeout),
                       [&]() { return seq.snd_una > currSeq; })) {
      // send succeed
      currSeq = 0;
    } else {
      if (closed) return;
      // send failed
      LOG_WARN("Send failed, rest retry: %d", retransCnt);
      if (retransCnt == 0) {
        retransCnt = tcpMaxRetrans;
        abanseqLock.lock();
        abandonedSeq.insert(currSeq);
        abanseqLock.unlock();
        SENDLST_POP(sendLock);
        seq.rcvAckWithLen(ti.ts.totalLen);
        currSeq = 0;
      } else {
        retransCnt--;
      }
    }
    seqLock.unlock();
    seqCv.notify_all();
  }
}

void TcpWorker::senderNonBlockLoop() {
  std::unique_lock sendNBLock(sendNonBlocklst_m, std::defer_lock);
  TcpItem ti;

  while (true) {
    // wait for something if no sengment
    sendNBLock.lock();
    sendNonBlockCv.wait(sendNBLock, [&] {
      if (closed)
        return true;
      else
        return sendNonBlockList.size() > 0;
    });
    if (closed) return;
    ti = sendNonBlockList.front();
    sendNonBlockList.pop();
    sendNBLock.unlock();
    Printer::printTcpItem(ti, true, "(NonBlock)");
    ti.ntoh();
    ti.setChecksum();
    Ip::sendIPPacket(ti.srcIp, ti.dstIp, IPPROTO_TCP, &ti.ts, ti.ts.totalLen);
  }
}  // namespace Tcp

ssize_t TcpWorker::read(u_char* buf, size_t nby) {
  std::unique_lock lock(recvbuf_m);

  // if is closed
  if (getSt() == TcpState::CLOSED) {
    LOG_WARN("Try to read a closed socket.");
    auto buf_size = recvBuf.size();
    if (buf_size == 0) {
      RET_SETERRNO(ENOTCONN);
    } else if (buf_size >= nby) {
      recvBuf.read(buf, nby);
      return nby;
    } else {
      recvBuf.read(buf, buf_size);
      return buf_size;
    }
  }

  recvCv.wait(lock, [&] { return closed || recvBuf.can_get(nby) > 0; });
  if (closed) {
    RET_SETERRNO(ENOTCONN);
  }
  nby = recvBuf.read(buf, nby);
  lock.unlock();
  return nby;
}

void TcpWorker::handler(TcpItem& recvti) {
  auto hdr = recvti.ts.hdr;
  std::unique_lock stlck(stSameCv_m);
  stSameCv.wait(stlck, [&] { return (st.load() == criticalSt.load()); });
  setCriticalSt(TcpState::INVAL);
  stlck.unlock();

  // no nothing if closed
  if (getSt() == TcpState::CLOSED) {
    return;
  }

  // ATTENTION: src and dst matched local address!!
  Socket::SocketAddr srcSaddr(recvti.dstIp, hdr.th_dport);
  Socket::SocketAddr dstSaddr(recvti.srcIp, hdr.th_sport);
  std::unique_lock<std::shared_mutex> lock(sendlst_m, std::defer_lock);

  // send duplicate ACK if rcving a larger sequence number
  if (syned.load() && hdr.th_seq > seq.rcv_nxt) {
    LOG_INFO("Send an old ACK");
    auto ti = buildAckItem(srcSaddr, dstSaddr, seq, seq_m, {}, seq.rcv_nxt);
    this->send(ti);
    setCriticalSt(getSt());
    return;
  }

  // send old ACK if rcving a smaller sequence number
  if (syned.load() && hdr.th_seq < seq.rcv_nxt) {
    LOG_INFO("Send a duplicated ACK");
    auto ti = buildAckItem(srcSaddr, dstSaddr, seq, seq_m, {},
                           hdr.th_seq + recvti.ts.dataLen);
    this->send(ti);
    setCriticalSt(getSt());
    return;
  }

  // if get RST, then closed
  if (WITHTYPE_RST(hdr)) {
    setSt(TcpState::CLOSED);
    return;
  }

  // if a meaningless ACK received, clear ACK flag
  if (WITHTYPE_ACK(hdr) && Sequence::equalTo(hdr.th_ack, seq.snd_nxt) &&
      Sequence ::equalTo(hdr.th_ack, seq.snd_una)) {
    hdr.th_flags -= TH_ACK;
  }

  switch (getSt()) {
    case TcpState::LISTEN: {
      // LISTEN --[rcv SYN, snd SYN/ACK]--> SYN_RCVD > `accept`
      if (ISTYPE_SYN(hdr)) {
        if (backlog == 0 || static_cast<int>(pendings.size()) < backlog) {
          tcp_seq seq = recvti.ts.hdr.th_seq;
          pendings.push(std::make_pair(dstSaddr, seq));
          acceptCv.notify_all();
        }
      }
      setCriticalSt(getSt());
      break;
    }
    case TcpState::SYN_SENT: {
      // STN_SENT --[rcv SYN/ACK, snd ACK]--> ESTAB > `connect`
      if (ISTYPE_SYN_ACK(hdr)) {
        if (seq.tryAndRcvAck(hdr)) {
          seq.initRcvIsn(hdr.th_seq);
          lock.lock();
          sendList.pop();
          lock.unlock();
          auto ti = Tcp::buildAckItem(srcSaddr, dstSaddr, seq, seq_m);
          this->send(ti);
          setCriticalSt(TcpState::ESTABLISHED);
          syned.store(true);
          seqCv.notify_all();
        } else {
          LOG_WARN("Error ACK with number %d, expected %d", hdr.ack,
                   seq.snd_nxt);
          setCriticalSt(getSt());
        }
      }
      // SYN_SENT --[rcv SYN, snd SYN/ACK]--> SYN_RCVD > `connect`
      else if (ISTYPE_SYN(hdr)) {
        seq.initRcvIsn(hdr.th_seq);
        lock.lock();
        sendList.pop();
        lock.unlock();
        setCriticalSt(TcpState::SYN_RECEIVED);
        syned.store(true);
        seqCv.notify_all();
      }
      break;
    }
    case TcpState::SYN_RECEIVED: {
      // SYN_RCVD --[rcv ACK]--> ESTAB > `connect` or `accept`
      if (ISTYPE_ACK(hdr)) {
        if (seq.tryAndRcvAck(hdr)) {
          lock.lock();
          sendList.pop();
          lock.unlock();
          setCriticalSt(TcpState::ESTABLISHED);
          syned.store(true);
          seqCv.notify_all();
        } else {
          LOG_WARN("Error ACK with number %d, expected %d", hdr.ack,
                   seq.snd_nxt);
          setCriticalSt(getSt());
        }
      } else {
        setCriticalSt(getSt());
      }
      break;
    }
    case TcpState::ESTABLISHED: {
      if (ISTYPE_ACK(hdr)) {
        if (seq.tryAndRcvAck(hdr)) {
          lock.lock();
          sendList.pop();
          lock.unlock();
          seqCv.notify_all();
        } else {
          LOG_WARN("Error ACK with number %d, expected %d", hdr.ack,
                   seq.snd_nxt);
        }
      }
      {  // get save and update rcv_nxt
        int len = recvti.ts.dataLen;
        if (len > 0) {
          bool pshflag = WITHTYPE_PUSH(hdr);
          recvBuf.write(recvti.ts.data, len, pshflag);
          recvCv.notify_all();
          if (!WITHTYPE_FIN(hdr)) {
            auto ti = buildAckItem(srcSaddr, dstSaddr, seq, seq_m, len);
            this->send(ti);
          }
        }
      }  // end save data
      // ESTAB --[rcv FIN, snd ACK]--> CLOSE_WAIT
      if (ISTYPE_FIN(hdr)) {
        auto ti = buildAckItem(srcSaddr, dstSaddr, seq, seq_m);
        this->send(ti);
        setSt(TcpState::CLOSE_WAIT);
      } else {
        setCriticalSt(getSt());
      }
      break;
    }
    case TcpState::FIN_WAIT_1: {
      {  // save data and update rcv_nxt
        int len = recvti.ts.dataLen;
        if (len > 0) {
          bool pshflag = WITHTYPE_PUSH(hdr);
          recvBuf.write(recvti.ts.data, len, pshflag);
          recvCv.notify_all();
          if (!WITHTYPE_FIN(hdr)) {
            auto ti = buildAckItem(srcSaddr, dstSaddr, seq, seq_m, len);
            this->send(ti);
          }
        }
      }  // end save data
      // FIN_WAIT_1 --[rcv FIN/ACK, snd ACK]--> TIME_WAIT > `close`
      if (ISTYPE_FIN_ACK(hdr)) {
        if (seq.tryAndRcvAck(hdr)) {
          lock.lock();
          sendList.pop();
          lock.unlock();
          setCriticalSt(TcpState::TIMED_WAIT);
          seqCv.notify_all();
        } else {
          LOG_WARN("Error ACK with number %d, expected %d", hdr.ack,
                   seq.snd_nxt);
          setCriticalSt(getSt());
        }
      }
      // FIN_WAIT_1 --[rcv ACK]--> FIN_WAIT_2 > `close`
      else if (ISTYPE_ACK(hdr)) {
        if (seq.tryAndRcvAck(hdr)) {
          lock.lock();
          sendList.pop();
          lock.unlock();
          setCriticalSt(TcpState::FIN_WAIT_2);
          seqCv.notify_all();
        } else {
          LOG_WARN("Error ACK with number %d, expected %d", hdr.ack,
                   seq.snd_nxt);
          setCriticalSt(getSt());
        }
      }
      // FIN_WAIT_1 --[rcv FIN, snd ACK]--> CLOSING > `close`
      else if (ISTYPE_FIN(hdr)) {
        auto ti = Tcp::buildAckItem(srcSaddr, dstSaddr, seq, seq_m);
        this->send(ti);
        setSt(TcpState::CLOSING);
      } else {
        setCriticalSt(getSt());
      }
      break;
    }
    case TcpState::FIN_WAIT_2: {
      if (ISTYPE_ACK(hdr)) {
        if (seq.tryAndRcvAck(hdr)) {
          lock.lock();
          sendList.pop();
          lock.unlock();
          seqCv.notify_all();
        } else {
          LOG_WARN("Error ACK with number %d, expected %d", hdr.ack,
                   seq.snd_nxt);
        }
      }  // end handle ACK
      {  // save data and update rcv_nxt
        int len = recvti.ts.dataLen;
        if (len > 0) {
          bool pshflag = WITHTYPE_PUSH(hdr);
          recvBuf.write(recvti.ts.data, len, pshflag);
          recvCv.notify_all();
          if (!WITHTYPE_FIN(hdr)) {
            auto ti = buildAckItem(srcSaddr, dstSaddr, seq, seq_m, len);
            this->send(ti);
          }
        }
      }  // end save data
      // ^ FIN_WAIT_2 --[rcv FIN, snd ACK]--> TIME_WAIT > `close`
      if (ISTYPE_FIN(hdr)) {
        setCriticalSt(TcpState::TIMED_WAIT);
      } else {
        setCriticalSt(getSt());
      }
      break;
    }
    case TcpState::CLOSING: {
      // ^ CLOSING --[rcv ACK]--> TIME_WAIT > `close`
      if (ISTYPE_ACK(hdr)) {
        if (seq.tryAndRcvAck(hdr)) {
          lock.lock();
          sendList.pop();
          lock.unlock();
          setCriticalSt(TcpState::TIMED_WAIT);
          seqCv.notify_all();
        } else {
          LOG_WARN("Error ACK with number %d, expected %d", hdr.ack,
                   seq.snd_nxt);
          setCriticalSt(getSt());
        }
      } else {
        setCriticalSt(getSt());
      }
      break;
    }
    case TcpState::LAST_ACK: {
      // LAST_ACK --[rcv ACK]--> CLOSED > `close`
      if (ISTYPE_ACK(hdr)) {
        if (seq.tryAndRcvAck(hdr)) {
          lock.lock();
          sendList.pop();
          lock.unlock();
          setCriticalSt(TcpState::CLOSED);
          seqCv.notify_all();
        } else {
          LOG_WARN("Error ACK with number %d, expected %d", hdr.ack,
                   seq.snd_nxt);
          setCriticalSt(getSt());
        }
      } else {
        setCriticalSt(getSt());
      }
      break;
    }
    default:
      break;
  }
}

}  // namespace Tcp
