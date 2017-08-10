#include "listener.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>

#include "util/log.h"
#include "util/network.h"

namespace tinyco {
Listener::Listener() : listenfd_(-1) {}

Listener::~Listener() {
  if (listenfd_ >= 0) close(listenfd_);
}

int TcpListener::Listen(const network::IP &ip, uint16_t port) {
  ip_ = ip;
  port_ = port;
  listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd_ < 0) {
    LOG_ERROR("socket error");
    return -__LINE__;
  }

  LOG_DEBUG("listenfd=%d|port=%u", listenfd_, port);

  sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = ip_.af_inet_ip;
  server.sin_port = htons(port_);

  if (network::SetNonBlock(listenfd_) < 0) {
    LOG_ERROR("set nonblock error");
    return -__LINE__;
  }

  if (network::SetReuseAddr(listenfd_)) {
    LOG("fail to setsockopt(SO_REUSEADDR)");
    return -__LINE__;
  }

  if (bind(listenfd_, (struct sockaddr *)&server, sizeof(server)) < 0) {
    LOG_ERROR("bind error");
    return -__LINE__;
  }

  if (listen(listenfd_, 5) < 0) {
    LOG_ERROR("listen error");
    return -__LINE__;
  }

  return 0;
}

int UdpListener::Listen(const network::IP &ip, uint16_t port) {
  ip_ = ip;
  port_ = port;

  listenfd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (listenfd_ < 0) {
    LOG("socket error");
    return -__LINE__;
  }

  struct sockaddr_in server, client;
  socklen_t clisocklen;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = ip_.af_inet_ip;
  server.sin_port = htons(port_);
  clisocklen = sizeof(client);

  if (network::SetNonBlock(listenfd_) < 0) {
    LOG_ERROR("set nonblock error");
    return -__LINE__;
  }

  if (bind(listenfd_, (struct sockaddr *)&server, sizeof(server)) < 0) {
    LOG("bind error");
    return -__LINE__;
  }

  return 0;
}
}
