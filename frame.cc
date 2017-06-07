#include "frame.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <algorithm>

namespace tinyco {
std::unordered_map<int, Thread *> Frame::io_wait_map_;  // wait on io
std::list<Thread *> Frame::thread_runnable_;
std::list<Thread *> Frame::thread_free_;
std::vector<Thread *> Frame::thread_pending_;
Thread *Frame::main_thread_;
Thread *Frame::running_thread_;
Thread *Frame::prunning_thread_;
struct event_base *Frame::base;
uint64_t Frame::last_loop_ts_ = 0;

struct ThreadPendingTimeComp {
  bool operator()(Thread *&a, Thread *&b) {
    return a->GetPendingDuration() < b->GetPendingDuration();
  }
};

bool Frame::Init() {
  auto m = new Thread;
  m->Init();
  m->SetContext(MainThreadLoop, NULL);
  main_thread_ = m;

  base = event_base_new();
  return true;
}

bool Frame::Fini() {
  delete running_thread_;
  running_thread_ = NULL;

  delete main_thread_;
  main_thread_ = NULL;

  for (auto it = thread_runnable_.begin(); it != thread_runnable_.end(); it++) {
    delete (*it);
  }

  for (auto it = thread_free_.begin(); it != thread_free_.end(); it++) {
    delete (*it);
  }

  event_base_free(base);
  return true;
}

uint64_t mstime() {
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (unsigned long long)(tv.tv_sec) * 1000 +
         (unsigned long long)(tv.tv_usec) / 1000;
}

int Frame::MainThreadLoop(void *arg) {
  struct timeval timeout;
  while (true) {
    LOG("thread_runnable_ size=%u|thread_free_ size=%u|thread_pending_size = "
        "%u",
        thread_runnable_.size(), thread_free_.size(), thread_pending_.size());

    UpdateLoopTimestamp();
    auto timeout = GetEventLoopTimeout();
    LOG("poll timeout=%us%uus|sleepsize=%u", timeout.tv_sec, timeout.tv_usec,
        thread_pending_.size());
    EventLoop(timeout);
    WakeupPendingThread();
    LOG("before schedule sleepsize=%u", thread_pending_.size());
    Schedule();
  }
}

Thread *Frame::PopPendingTop() {
  auto *t = *thread_pending_.begin();
  thread_pending_.erase(thread_pending_.begin());
  make_heap(thread_pending_.begin(), thread_pending_.end(),
            ThreadPendingTimeComp());
  return t;
}

void Frame::WakeupPendingThread() {
  auto nowms = GetLastLoopTimestamp();
  if (!thread_pending_.empty()) {
    auto n = *thread_pending_.begin();
    auto sz = thread_pending_.size();
    while (sz-- && n->GetPendingDuration() >= nowms) {
      n = PopPendingTop();
      n->Pending(0);
      thread_runnable_.push_back(n);

      n = *thread_pending_.begin();
    }
  }
}

timeval Frame::GetEventLoopTimeout() {
  uint64_t nowms = GetLastLoopTimestamp();
  struct timeval timeout = {0, 100000};
  if (!thread_pending_.empty()) {
    auto mint = *thread_pending_.begin();

    // thread need to be wake up
    if (mint->GetPendingDuration() < nowms) {
      timeout.tv_usec = 0;
      return timeout;
    }

    timeout.tv_sec = (mint->GetPendingDuration() - nowms) / 1000llu;
    timeout.tv_usec =
        (mint->GetPendingDuration() - nowms - timeout.tv_sec * 1000llu) *
        1000llu;
    LOG("duration=%llu|sec=%llu|usec=%llu|nowms=%llu",
        mint->GetPendingDuration(), timeout.tv_sec, timeout.tv_usec, nowms);
  }
  return timeout;
}

int Frame::EventLoop(const timeval &tv) {
  event_base_loopexit(base, &tv);
  return event_base_loop(base, 0);
}

int Frame::UdpSendAndRecv(const std::string &sendbuf,
                          struct sockaddr_in &dest_addr, std::string *recvbuf) {
  int ret = 0;
  char recvbuf_[2048] = {0};
  auto recvlen = sizeof(recvbuf_);
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) return -1;
  flags = flags | O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);

  if ((ret = sendto(fd, sendbuf.c_str(), sendbuf.size(), 0,
                    (struct sockaddr *)&dest_addr, sizeof(dest_addr))) < 0) {
    LOG("sendto error");
    return -1;
  }

  socklen_t sockaddr_len = sizeof(dest_addr);
  struct event *ev = NULL;
  while (true) {
    if ((ret = recvfrom(fd, recvbuf_, sizeof(recvbuf), 0,
                        (struct sockaddr *)&dest_addr, &sockaddr_len)) < 0) {

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        LOG("schedule!!! fd = %d|ret = %d|recvbuf_=%s", fd, ret, recvbuf_);

        if (!ev) ev = event_new(NULL, -1, 0, NULL, NULL);

        event_assign(ev, base, fd, EV_READ, SocketReadable, ev);
        event_add(ev, NULL);  // add timeout
        io_wait_map_[fd] = running_thread_;
        running_thread_->Schedule();
        Schedule();

        if (ev->ev_res & EV_TIMEOUT) {
          break;
        } else if (ev->ev_res & EV_READ) {
          continue;
        }
        break;
      }
      break;
    }

    if (ret > 0) {
      recvbuf->assign(recvbuf_, ret);
      break;
    }
  }

  if (ev) event_free(ev);

  return 0;
}

void Frame::SocketReadable(int fd, short events, void *arg) {
  if (events & EV_TIMEOUT) {
    int ret = event_del(static_cast<event *>(arg));
    LOG("del event:%d", ret);
  }

  thread_runnable_.push_back(io_wait_map_[fd]);
  io_wait_map_.erase(fd);
  event_base_loopbreak(Frame::base);
}

int HandleProcess(void *arg) {
  auto *w = static_cast<Work *>(arg);
  w->Run();
  delete w;

  // recycle the thread
  Frame::RecycleRunningThread();
  Frame::Schedule();
  return 0;
}

void Frame::RecycleRunningThread() {
  thread_free_.push_back(running_thread_);
  running_thread_ = NULL;
}

int Frame::CreateThread(Work *w) {
  auto *t = new Thread;
  t->Init();
  t->SetContext(HandleProcess, w);
  thread_runnable_.push_back(t);
}

int Frame::Schedule() {
  if (thread_runnable_.empty()) {
    main_thread_->RestoreContext();
  } else {
    running_thread_ = *(thread_runnable_.begin());
    thread_runnable_.pop_front();
    running_thread_->RestoreContext();
  }
  return 0;
}

void Frame::Sleep(uint32_t ms) {
  if (running_thread_) {
    running_thread_->Pending(mstime() + ms);
    thread_pending_.push_back(running_thread_);
    running_thread_->Schedule();
    running_thread_ = NULL;
  }

  Schedule();
}

void Frame::PendThread(Thread *t) {
  thread_pending_.push_back(t);
  make_heap(thread_pending_.begin(), thread_pending_.end(),
            ThreadPendingTimeComp());
}

void Frame::UpdateLoopTimestamp() { last_loop_ts_ = mstime(); }
}
