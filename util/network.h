
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

struct EndPoint {
  IP ip;
  uint16_t port;

  EndPoint() : ip(IP{0}), port(0) {}
  EndPoint(IP ip, uint16_t port) : ip(ip), port(port) {}

  std::string ToString() { return ntoa(ip) + ":" + std::to_string(port); }
};
}
}
#endif
