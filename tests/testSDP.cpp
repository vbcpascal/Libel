#include "api.h"
#include "router.h"
#include "sdp.h"

int main(int argc, char* argv[]) {
  api::init();
  api::addAllDevice();
  api::initRouter();

  Device::deviceMgr.keepReceiving();
  return 0;
}