#ifndef FRAME_H_
#define FRAME_H_

#include <inttypes.h>
#include <unordered_map>
#include <list>
#include <set>
#include <vector>
#include <memory>
#include <fcntl.h>
#include <unistd.h>

#include "thread.h"
#include "work.h"
#include "mutex.h"
#include "event2/event.h"
#include "util/defer.h"
#include "util/log.h"
#include "listener.h"

namespace tinyco {

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

  // it's very useful before mainloop of your program
  static void InitHereAsNewThread() {
    auto t = new Thread();
    t->Init();
    t->Schedule();
    running_thread_ = t;
  }

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
  static struct event_base *base;

 private:
  Frame() {}
  virtual ~Frame() {}

  static uint64_t last_loop_ts_;
};
}

#endif
