/**
 * @file socketstate.h
 * @author guanzhichao
 * @brief
 * @version 0.1
 * @date 2019-11-02
 *
 * @copyright Copyright (c) 2019
 *
 */

#ifndef TCPSTATE_H_
#define TCPSTATE_H_

#include <netinet/tcp_fsm.h>

#include "massert.h"
#include "type.h"

namespace Tcp {

enum class TcpState : int {
  INVAL = -1,
  CLOSED = 0,
  LISTEN,
  SYN_SENT,
  SYN_RECEIVED,
  ESTABLISHED,
  CLOSE_WAIT,
  FIN_WAIT_1,
  CLOSING,
  LAST_ACK,
  FIN_WAIT_2,
  TIMED_WAIT,
};

}  // namespace Tcp

#endif