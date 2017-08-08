#include "network.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */

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
}
}
