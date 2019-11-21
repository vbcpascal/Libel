/**
 * @file tcpstate.h
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-11-17
 *
 * @brief State of TCP(Socket)
 *
 */

#ifndef TCPSTATE_H_
#define TCPSTATE_H_

#include "massert.h"
#include "type.h"

namespace Tcp {

#define TCP_STATE_SET \
  X(INVAL)            \
  X(CLOSED)           \
  X(LISTEN)           \
  X(SYN_SENT)         \
  X(SYN_RECEIVED)     \
  X(ESTABLISHED)      \
  X(CLOSE_WAIT)       \
  X(FIN_WAIT_1)       \
  X(CLOSING)          \
  X(LAST_ACK)         \
  X(FIN_WAIT_2)       \
  X(TIMED_WAIT)

enum class TcpState : int {
#define X(STATENAME) STATENAME,
  TCP_STATE_SET
#undef X
};

std::string stateToStr(TcpState st);
}  // namespace Tcp

#endif