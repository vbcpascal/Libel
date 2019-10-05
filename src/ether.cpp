#include "ether.h"
#include <map>

namespace MAC {

std::string toString(const u_char* mac) {
  char cstr[18] = "";
  sprintf(cstr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
          mac[4], mac[5]);
  return std::string(cstr);
}

void printMAC(const u_char* mac, bool newline) {
  printf("%s", toString(mac).c_str());
  if (newline) printf("\n");
}

void strtoMAC(u_char* mac, const char* str) {
  int tmp[6] = {0};
  sscanf(str, "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4],
         &tmp[5]);
  for (int i = 0; i < 6; ++i) mac[i] = static_cast<u_char>(tmp[i]);
}

}  // namespace MAC

std::string etherToStr(uint16_t type) {
  std::string s;
  switch (type) {
    case ETHERTYPE_IP:
      s = "IPV4  ";
      break;
    case ETHERTYPE_IPV6:
      s = "IPV6  ";
      break;
    case ETHERTYPE_VLAN:
      s = "VLAN  ";
      break;
    default:
      s = "";
      // s = std::to_string(type);
  }
  return s;
};

EtherFrame::EtherFrame() {
  frame.header.ether_type = 0;
  std::memset(frame.header.ether_dhost, 0, ETHER_ADDR_LEN);
  std::memset(frame.header.ether_shost, 0, ETHER_ADDR_LEN);
  std::memset(frame.payload, 0, ETHER_MAX_LEN);
  std::memset(frame.crc, 0, ETHER_CRC_LEN);
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
  std::memcpy(&frame, buf, len);
}

void EtherFrame::printFrame(int col, int option) {
  int b = 0;

  if ((option >> (b++)) & 1) {
    printf("[ \033[33mPACKET\033[0m ] \t");
  }

  if ((option >> (b++)) & 1) {
    MAC::printMAC(frame.header.ether_shost, false);
    printf(" > ");
    MAC::printMAC(frame.header.ether_dhost, false);
  }  // print mac

  if ((option >> (b++)) & 1) {
    std::string s = etherToStr(frame.header.ether_type);
    if (s == "")
      printf("\ttype: 0x%04x", frame.header.ether_type);
    else
      printf("\ttype: %s", s.c_str());
  }  // print type

  if ((option >> (b++)) & 1) {
    printf("\tlen: %d\n", len);
  }  // print length

  if (col <= 0) return;

  for (int i = 0; i < len; ++i) {
    if (i != 0 && i % (8 * col) == 0)
      printf("\n");
    else if (i != 0 && i % 8 == 0)
      printf("\t");

    printf("%02x ", frame.payload[i]);
  }
  printf("\n");
}
