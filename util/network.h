
#ifndef NETWORK_H_
#define NETWORK_H_

#include <inttypes.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace tinyco {
namespace network {

union IP {
  uint32_t af_inet_ip;
  uint64_t af_inet6_ip;
};

bool GetEthAddr(const char *eth, IP *ip);

int SetNonBlock(int fd);

int SetReuseAddr(int fd);

std::string ntoa(const IP &ip);

std::string InetAddrToString(const sockaddr_in &addr);
}
}
#endif
