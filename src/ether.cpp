#include "ether.h"

namespace MAC {

std::string toString(u_char* mac) {
  char cstr[18] = "";
  sprintf(cstr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
          mac[4], mac[5]);
  return std::string(cstr);
}

void printMAC(u_char* mac, bool newline) {
  printf("%s", toString(mac).c_str());
  //   printf("%02x", mac[0]);
  //   for (int i = 1; i < ETHER_ADDR_LEN; ++i) {
  //     printf(":%02x", mac[i]);
  //   }
  if (newline) printf("\n");
}

void strtoMAC(u_char* mac, const char* str) {
  int tmp[6] = {0};
  sscanf(str, "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4],
         &tmp[5]);
  for (int i = 0; i < 6; ++i) mac[i] = static_cast<u_char>(tmp[i]);
}

}  // namespace MAC

EtherFrame::EtherFrame() {
  std::memset(frame, 0, ETHER_MAX_LEN);
  std::memset(header.ether_dhost, 0, ETHER_ADDR_LEN);
  std::memset(header.ether_shost, 0, ETHER_ADDR_LEN);
}

EtherFrame::EtherFrame(const void* buf, int l) : len(l) {
  if (len < ETHER_HDR_LEN) {
    LOG(ERR, "packet length is too small. length : %d", len);
    len = 0;
    return;
  } else if (len > ETHER_MAX_LEN) {
    LOG(ERR, "packet length is too large. length : %d", len);
    len = 0;
    return;
  }
  std::memcpy(frame, buf, len);
  std::memcpy(&header, buf, ETHER_HDR_LEN);
}

void EtherFrame::printFrame(int col) {
  printf("[ \033[33mPACKET\033[0m ] \t");
  MAC::printMAC(header.ether_shost, false);
  printf(" > ");
  MAC::printMAC(header.ether_dhost, false);
  printf("\ttype: 0x%04x", header.ether_type);
  printf("\tlen: %d\n", len);
  if (col <= 0) return;

  for (int i = 0; i < len; ++i) {
    if (i != 0 && i % (8 * col) == 0)
      printf("\n");
    else if (i != 0 && i % 8 == 0)
      printf("\t");

    printf("%02x ", frame[i]);
  }
  printf("\n");
}
