#include "ip.h"

uint16_t checksum(uint16_t *addr, int len) {
  int nleft = len;
  int sum = 0;

  uint16_t *w = addr;
  uint16_t answer = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= sizeof(uint16_t);
  }

  if (nleft == 1) {
    *(uint8_t *)(&answer) = *(uint8_t *)w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  answer = ~sum;
  return (answer);
}

#define CHECK(HDR...)                                                        \
  {                                                                          \
    auto len = std::vector<int>(HDR).size();                                 \
    const u_char hdr[] = HDR;                                                \
    /*auto res = Ip::getChecksum(hdr, len);*/                                \
    auto res = checksum((uint16_t *)hdr, len);                               \
    if (res == 0)                                                            \
      LOG_INFO("case %d: with size %3ld Checksum correct.", caseNum++, len)  \
    else                                                                     \
      LOG_ERR(                                                               \
          "case %d: with size %3ld Checksum validation failed. res: 0x%x, ", \
          caseNum++, len, res);                                              \
  }

int caseNum = 1;

int main() {
  CHECK({
      0x45, 0x20, 0x00, 0x28, 0x56, 0x78, 0x40, 0x00, 0x33, 0x06,
      0xda, 0x24, 0x3c, 0xd5, 0x16, 0x43, 0xc0, 0xa8, 0x03, 0x53,

  });
  CHECK({
      0x45, 0x00, 0x00, 0x28, 0x00, 0x00, 0x40, 0x00, 0x40, 0x06,
      0x24, 0x34, 0xc0, 0xa8, 0x03, 0x53, 0x2a, 0x9f, 0x28, 0x02,
  });
  CHECK({
      0x0a, 0x64, 0x01, 0x01, /* Src Ip */
      0x0a, 0x64, 0x01, 0x02, /* Dst Ip */
      0x00, 0x06, 0x00, 0x28, /* Zero, Proto, 2-Length */
      0xed, 0x08, 0x08, 0x00, /* Ports */
      0x1a, 0x4c, 0xc1, 0x4b, /* Sequence number */
      0x00, 0x00, 0x00, 0x00, /* ACK number */
      0xa0, 0x02, 0xfa, 0xf0, /* [offset,0], flags, win */
      0x6d, 0xd1, 0x00, 0x00, /* sum, urp */
      0x02, 0x04, 0x05, 0xb4, /* option 1 */
      0x04, 0x02, 0x08, 0x0a, /* option 2 */
      0x30, 0x9d, 0xc7, 0x35, /* option 3 */
      0x00, 0x00, 0x00, 0x00, /* option 4 */
      0x01, 0x03, 0x03, 0x07, /* option 5 */
  });
  return 0;
}