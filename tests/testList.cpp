/**
 * @file testList.cpp
 * @author guanzhichao (vbcpascal@gmail.com)
 * @version 0.1
 * @date 2019-11-22
 *
 * @brief Test: list all device in the host.
 *
 */

#include "api.h"

int main() {
  Device::deviceMgr.addAllDevice(false);
  return 0;
}
