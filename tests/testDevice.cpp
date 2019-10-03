#include "device.h"
#include "packetio.h"
#include "type.h"

int myCallback(const void* buf, int len, int id) {
  auto frame = EtherFrame(buf, len);
  // LOG(INFO, "get packet with len: %d, from id: %d.", len, id);
  frame.printFrame(true);
  return 0;
}

int main() {
  setFrameReceiveCallback(&myCallback);

  pcap_if_t* devs;
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_findalldevs(&devs, errbuf);
  while (devs != NULL) {
    addDevice(devs->name);
    devs = devs->next;
  }
  LOG(INFO, "add succeed.");
  int id = findDevice("en0");
  LOG("RESULT", "find device. id: %d: ", id);

  keepReceiving();
  return 0;
}