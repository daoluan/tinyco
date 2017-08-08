
#ifndef NETWORK_H_
#define NETWORK_H_

#include <inttypes.h>

namespace tinyco {
namespace network {

union IP {
  uint32_t af_inet_ip;
  uint64_t af_inet6_ip;
};

bool GetEthAddr(const char *eth, IP *ip);
}
}
#endif
