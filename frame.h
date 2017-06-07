#ifndef FRAME_H_
#define FRAME_H_

#include <inttypes.h>
#include <unordered_map>
#include <event2/event.h>
#include <list>
#include <vector>
#include <memory>

#include "thread.h"
#include "work.h"

namespace tinyco {

#define LOG(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)

class IsCompleteBase {
 public:
  IsCompleteBase() {}
  virtual ~IsCompleteBase() {}

  int operator()(char *buffer, uint32_t buffer_len) { return 0; }
};

class Frame {
 public:
  static bool Init();
  static bool Fini();

 public:
  static int UdpSendAndRecv(const std::string &sendbuf,
                            struct sockaddr_in &dest_addr,
                            std::string *recvbuf);
  static int TcpSendAndRecv(int fd, const std::string &sendbuf,
                            std::string *recvbuf, IsCompleteBase *is_complete);
  static int CreateThread(Work *w);
  static void Sleep(uint32_t ms);
  static int Schedule();
  static void RecycleRunningThread();

 private:
  static void SocketReadable(int fd, short events, void *arg);
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
}

#endif
