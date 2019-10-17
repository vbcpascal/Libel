#include "type.h"

bool operator<(ip_addr a, ip_addr b) { return a.s_addr < b.s_addr; }
bool operator==(ip_addr a, ip_addr b) { return a.s_addr == b.s_addr; }