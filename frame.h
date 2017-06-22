#ifndef FRAME_H_
#define FRAME_H_

#include <inttypes.h>
#include <unordered_map>
#include <list>
#include <vector>
#include <memory>
#include <fcntl.h>
#include <unistd.h>

#include "thread.h"
#include "work.h"
#include "mutex.h"
#include "event2/event.h"

namespace tinyco {

#define LOG(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)

class IsCompleteBase {
 public:
  IsCompleteBase() {}
  virtual ~IsCompleteBase() {}

  virtual int CheckPkg(const char *buffer, uint32_t buffer_len) { return 0; }
};

class Frame {
 public:
  static bool Init();
  static bool Fini();

 public:
  static int UdpSendAndRecv(const std::string &sendbuf,
                            struct sockaddr_in &dest_addr,
                            std::string *recvbuf);
  static int TcpSendAndRecv(const void *sendbuf, size_t sendlen, void *recvbuf,
                            size_t *recvlen, IsCompleteBase *is_complete);
  static int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  static int connect(int sockfd, const struct sockaddr *addr,
                     socklen_t addrlen);
  static int send(int sockfd, const void *buf, size_t len, int flags);
  static int recv(int sockfd, void *buf, size_t len, int flags);
  static int sendto(int sockfd, const void *buf, size_t len, int flags,
                    const struct sockaddr *dest_addr, socklen_t addrlen);
  static ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                          struct sockaddr *src_addr, socklen_t *addrlen);
  static int CreateThread(Work *w);
  static void Sleep(uint32_t ms);
  static int Schedule();
  static void RecycleRunningThread();

  template <class W>
  static int TcpSrv(uint32_t ip, uint16_t port);

  template <class W>
  static int UdpSrv(uint32_t ip, uint16_t port);

 private:
  static void SocketReadOrWrite(int fd, short events, void *arg);
  static int MainThreadLoop(void *arg);
  static void PendThread(Thread *t);
  static Thread *PopPendingTop();
  static void WakeupPendingThread();
  static timeval GetEventLoopTimeout();
  static int EventLoop(const timeval &tv);
  static void UpdateLoopTimestamp();
  static uint64_t GetLastLoopTimestamp() { return last_loop_ts_; }

  static std::unordered_map<int, Thread *> io_wait_map_;
  static std::list<Thread *> thread_runnable_;
  static std::list<Thread *> thread_free_;
  static std::vector<Thread *> thread_pending_;
  static Thread *main_thread_;
  static Thread *running_thread_;
  static Thread *prunning_thread_;
  static struct event_base *base;

 private:
  Frame() {}
  virtual ~Frame() {}

  static uint64_t last_loop_ts_;
};

class defer {
  std::function<void()> t;

 public:
  defer(std::function<void()> &&t) : t(t) {}
  ~defer() { t(); }
};

template <class W>
class TcpSrvWork : public Work {
 public:
  TcpSrvWork(uint32_t ip, uint16_t port) : ip_(ip), port_(port) {}
  virtual ~TcpSrvWork() {}

  int Run() {
    int ret = RunTcpSrv();
    if (ret < 0) {
      LOG("server error: %d", ret);
      exit(EXIT_FAILURE);
    }
  }

 private:
  int RunTcpSrv() {
    auto listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
      LOG("socket error");
      return -__LINE__;
    }

    defer d([=] { close(listenfd); });

    struct sockaddr_in server, client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = ip_;
    server.sin_port = htons(port_);

    auto flags = fcntl(listenfd, F_GETFL, 0);
    if (flags < 0) {
      LOG("fcntl get error");
      return -__LINE__;
    }

    flags = flags | O_NONBLOCK;
    fcntl(listenfd, F_SETFL, flags);

    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0) {
      LOG("fail to setsockopt(SO_REUSEADDR)");
      return -__LINE__;
    }

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
      LOG("bind error");
      return -__LINE__;
    }

    if (listen(listenfd, 5) < 0) {
      LOG("listen error");
      return -__LINE__;
    }

    // try to lock when multiprocss on
    auto masterpid = getpid();
    FileMtx fm;
    if (fm.OpenLockFile(std::string("/tmp/tinyco_lf_") +
                        std::to_string(masterpid)) < 0) {
      return -__LINE__;
    }

    for (auto i = 0; i < 3; i++) {  // todo configurable
      if (fork() > 0)               // child
        break;
    }

    while (true) {
      if (fm.TryLock() < 0) {
        // lock error, schedule out
        Frame::Sleep(500);
        continue;
      }

      socklen_t socklen = sizeof(client);
      auto fd = Frame::accept(listenfd, (struct sockaddr *)&client, &socklen);
      if (fd < 0) {
        LOG("Frame::accept error: ret=%d", fd);
        return -__LINE__;
      }

      auto flags = fcntl(fd, F_GETFL, 0);
      if (flags < 0) {
        LOG("fcntl get error");
        return -__LINE__;
      }

      flags = flags | O_NONBLOCK;
      fcntl(fd, F_SETFL, flags);

      auto w = new W;
      w->SetFd(fd);
      if (fd < 0) {
        continue;
      }

      fm.Unlock();
      Frame::CreateThread(w);
    }

    return 0;
  }
  uint32_t ip_;
  uint16_t port_;
};

struct UdpReqInfo {
  uint32_t sockfd;
  std::string reqpkg;
  sockaddr_in srcaddr;
  sockaddr_in dstaddr;
};

class UdpReqWork : public Work {
 public:
  UdpReqWork() {}
  virtual ~UdpReqWork() {}
  void SetReqInfo(const UdpReqInfo &req) { req_ = req; }
  int Run() = 0;
  int Reply(const std::string &rsp) {
    return Frame::sendto(req_.sockfd, rsp.data(), rsp.size(), 0,
                         (sockaddr *)&req_.srcaddr, sizeof(req_.srcaddr));
  }

 protected:
  UdpReqInfo req_;
};

template <class W>
class UdpSrvWork : public Work {
 public:
  UdpSrvWork(uint32_t ip, uint16_t port) : ip_(ip), port_(port) {}
  virtual ~UdpSrvWork() {}
  int Run() {
    int ret = RunUdpSrv();
    if (ret < 0) {
      LOG("server error: %d", ret);
      exit(EXIT_FAILURE);
    }
  }

 private:
  int RunUdpSrv() {
    auto listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenfd < 0) {
      LOG("socket error");
      return -__LINE__;
    }

    defer d([=] { close(listenfd); });

    struct sockaddr_in server, client;
    socklen_t clisocklen;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = ip_;
    server.sin_port = htons(port_);
    clisocklen = sizeof(client);

    auto flags = fcntl(listenfd, F_GETFL, 0);
    if (flags < 0) {
      LOG("fcntl get error");
      return -__LINE__;
    }

    flags = flags | O_NONBLOCK;
    fcntl(listenfd, F_SETFL, flags);

    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0) {
      LOG("fail to setsockopt(SO_REUSEADDR)");
      return -__LINE__;
    }

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
      LOG("bind error");
      return -__LINE__;
    }

    char recvbuf[65536];

    while (true) {
      int ret = Frame::recvfrom(listenfd, recvbuf, sizeof(recvbuf), 0,
                                (struct sockaddr *)&client, &clisocklen);
      if (ret > 0) {
        // create tread to handle request
        UdpReqInfo req;
        req.srcaddr = client;
        req.dstaddr = server;
        req.reqpkg.assign(recvbuf, ret);
        req.sockfd = listenfd;

        auto w = new W();
        w->SetReqInfo(req);

        Frame::CreateThread(w);
      } else if (ret < 0) {
        LOG("Frame::recvfrom error: %d", ret);
        return -__LINE__;
      }
    }

    return 0;
  }

  uint32_t ip_;
  uint16_t port_;
};

template <class W>
int Frame::TcpSrv(uint32_t ip, uint16_t port) {
  Frame::CreateThread(new TcpSrvWork<W>(ip, port));
  Frame::Schedule();
}

template <class W>
int Frame::UdpSrv(uint32_t ip, uint16_t port) {
  Frame::CreateThread(new UdpSrvWork<W>(ip, port));
  Frame::Schedule();
}
}

#endif
