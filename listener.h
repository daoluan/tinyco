#ifndef LISTENER_H_
#define LISTENER_H_

#include "./dns/dns_resolve.h"
#include "util/network.h"
#include "mutex.h"

#include <unistd.h>

namespace tinyco {
class Listener {
 public:
  Listener();
  virtual ~Listener();
  virtual int Listen(const network::IP &ip, uint16_t port) = 0;
  virtual Mutex *GetMtx() { return mtx_; }
  int GetSocketFd() const { return listenfd_; }
  void Destroy() {
    if (listenfd_ >= 0) close(listenfd_);
  }
  network::IP GetIP() const { return ip_; }
  uint16_t GetPort() const { return port_; }

 protected:
  int listenfd_;
  network::IP ip_;
  uint16_t port_;
  Mutex *mtx_;
};

class TcpListener : public Listener {
 public:
  TcpListener() { mtx_ = new AtomicMtx(); }
  virtual int Listen(const network::IP &ip, uint16_t port);
};

class UdpListener : public Listener {
 public:
  UdpListener() { mtx_ = new AtomicMtx(); }
  virtual int Listen(const network::IP &ip, uint16_t port);
};
}

#endif
