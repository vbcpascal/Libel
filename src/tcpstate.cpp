#include "tcpstate.h"

namespace Tcp {
std::string stateToStr(TcpState st) {
  switch (st) {
#define X(STATENAME)        \
  case TcpState::STATENAME: \
    return #STATENAME;
    TCP_STATE_SET
#undef X
    default:
      return "";
  }
  return "";
}
}  // namespace Tcp