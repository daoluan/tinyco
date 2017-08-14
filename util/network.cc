#include "network.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

namespace tinyco {
namespace network {

bool GetEthAddr(const char *eth, IP *ip) {
  int fd;
  struct ifreq ifr;

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  /* I want to get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;

  /* I want IP address attached to "eth0" */
  strncpy(ifr.ifr_name, eth, IFNAMSIZ - 1);

  ioctl(fd, SIOCGIFADDR, &ifr);

  close(fd);

  ip->af_inet_ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

  return true;
}

int SetNonBlock(int fd) {
  auto flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return flags;
  }

  flags = flags | O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

int SetReuseAddr(int fd) {
  int enable = 1;
  return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
}

std::string ntoa(const IP &ip) {
  in_addr ia;
  ia.s_addr = ip.af_inet_ip;
  return inet_ntoa(ia);
}

std::string InetAddrToString(const sockaddr_in &addr) {
  const std::string &ip = inet_ntoa(addr.sin_addr);
  return ip + ":" + std::to_string(ntohs(addr.sin_port));
}
}
}
