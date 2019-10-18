#include "ether.h"

#include <cstdint>
#include <map>

namespace MAC {

bool MacAddr::operator==(const MacAddr& m) {
  return isSameMacAddr(addr, m.addr);
}

const MacAddr& MacAddr::operator=(const MacAddr& m) {
  memcpy(addr, m.addr, ETHER_ADDR_LEN);
  return *this;
}

std::string toString(const u_char* mac) {
  char cstr[18] = "";
  sprintf(cstr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
          mac[4], mac[5]);
  std::string s(cstr);
  return s;
}

void strtoMAC(u_char* mac, const char* str) {
  int tmp[6] = {0};
  sscanf(str, "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4],
         &tmp[5]);
  for (int i = 0; i < 6; ++i) mac[i] = static_cast<u_char>(tmp[i]);
}

bool isSameMacAddr(const u_char* macA, const u_char* macB) {
  for (int i = 0; i < 6; ++i)
    if (macA[i] != macB[i]) return false;
  return true;
}

bool isBroadcast(const u_char* mac) {
  return isSameMacAddr(mac, Ether::broadcastMacAddr);
}

bool isBroadcast(const MacAddr& mac) {
  return isSameMacAddr(mac.addr, Ether::broadcastMacAddr);
}

}  // namespace MAC

std::string etherToStr(uint16_t type) {
  std::string s;
  switch (type) {
    case ETHERTYPE_IP:
      s = "IPV4 ";
      break;
    case ETHERTYPE_IPV6:
      s = "IPV6 ";
      break;
    case ETHERTYPE_ARP:
      s = "ARP  ";
      break;
    default:
      s = "";
      // s = std::to_string(type);
  }
  return s;
};

namespace Ether {

const u_char broadcastMacAddr[6] = {255, 255, 255, 255, 255, 255};
const u_char zeroMacAddr[6] = {0, 0, 0, 0, 0, 0};

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

}  // namespace Ether

namespace Printer {

void printMAC(const u_char* mac, const std::string end) {
  printf("%s%s", MAC::toString(mac).c_str(), end.c_str());
  fflush(stdout);
}

void printEtherFrame(const Ether::EtherFrame& ef, int col, int option) {
  int b = 0, len = ef.len;
  auto& frame = ef.frame;

  if ((option >> (b++)) & 1) {
    printf("[ \033[33mPACKET\033[0m ] \t");
  }

  if ((option >> (b++)) & 1) {
    Printer::printMAC(frame.header.ether_shost, ">");
    Printer::printMAC(frame.header.ether_dhost, "");
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

  u_char* frameBuf = new u_char[len];
  memcpy(frameBuf, &frame, len);
  for (int i = 0; i < len; ++i) {
    if (i != 0 && i % (8 * col) == 0)
      printf("\n");
    else if (i != 0 && i % 8 == 0)
      printf("\t");

    printf("%02x ", frameBuf[i]);
  }
  printf("\n");
  free(frameBuf);
}

void print(const u_char* buf, int len, int placeholder) {
  int col = 2;
  int index = 0;
  for (int i = 0; i < len + placeholder; ++i) {
    if (i != 0 && i % (8 * col) == 0)
      printf("\n");
    else if (i != 0 && i % 8 == 0)
      printf("\t");

    if (placeholder) {
      printf("   ");
      placeholder--;
    } else {
      printf("%02x ", buf[index++]);
    }
  }
  printf("\n");
}

}  // namespace Printer
