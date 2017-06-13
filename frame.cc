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
    UpdateLoopTimestamp();
    auto timeout = GetEventLoopTimeout();
    EventLoop(timeout);
    WakeupPendingThread();
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
      n->SetState(Thread::TS_RUNNABLE);
      thread_runnable_.push_back(n);

      n = *thread_pending_.begin();
    }
  }
}

timeval Frame::GetEventLoopTimeout() {
  auto nowms = GetLastLoopTimestamp();
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
    return -1;
  }

  socklen_t sockaddr_len = sizeof(dest_addr);
  struct event *ev = NULL;
  while (true) {
    if ((ret = ::recvfrom(fd, recvbuf_, sizeof(recvbuf), 0,
                          (struct sockaddr *)&dest_addr, &sockaddr_len)) < 0) {

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (!ev) ev = event_new(NULL, -1, 0, NULL, NULL);
        event_assign(ev, base, fd, EV_READ, SocketReadable, ev);
        event_add(ev, NULL);  // add timeout
        io_wait_map_[fd] = running_thread_;
        running_thread_->Schedule();
        running_thread_->SetState(Thread::TS_STOP);
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

int Frame::TcpSendAndRecv(const void *sendbuf, size_t sendlen, void *recvbuf,
                          size_t *recvlen, IsCompleteBase *is_complete) {
  int nsend = 0;
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) return -1;
  flags = flags | O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);

  struct event *ev = NULL;
  while (nsend == sendlen) {
    int ret = send(fd, static_cast<const char *>(sendbuf) + nsend,
                   sendlen - nsend, 0);
    if (ret > 0) {
      nsend += ret;
    }
    if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    } else if (ret < 0) {
      return -1;
    } else if (0 == ret) {
      return -1;
    }
  }

  size_t recvd = 0;
  while (true) {
    int ret = recv(fd, recvbuf, *recvlen, 0);
    if (ret > 0) {
      recvd += ret;
      // ok
      if (is_complete->CheckPkg(static_cast<char *>(recvbuf), recvd) == 0) {
        *recvlen = recvd;
        return 0;
      }

      continue;
    } else if (0 == ret) {  // reset by peer
      return -2;
    } else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      continue;
    } else {
      return -2;
    }
  }
  return 0;
}

int Frame::connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  struct event *ev = NULL;
  struct timeval timeout = {0, 100000};
  int ret = ::connect(sockfd, addr, addrlen);
  if (EINPROGRESS == errno) {
    if (!ev) ev = event_new(NULL, -1, 0, NULL, NULL);
    event_assign(ev, base, sockfd, EV_WRITE, SocketReadable, ev);
    event_add(ev, &timeout);  // add timeout
    io_wait_map_[sockfd] = running_thread_;
    running_thread_->Schedule();
    running_thread_->SetState(Thread::TS_STOP);
    Schedule();

    if (ev->ev_res & EV_TIMEOUT) {
      return -1;
    } else if (ev->ev_res != EV_WRITE) {
      return -1;
    }
  } else {
    return -1;
  }

  return 0;
}

int Frame::send(int sockfd, const void *buf, size_t len, int flags) {
  struct event *ev = NULL;
  size_t nsend = 0;
  while (nsend < len) {
    int ret =
        ::send(sockfd, static_cast<const char *>(buf) + nsend, len - nsend, 0);
    if (ret > 0) {
      nsend += ret;
    } else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      if (!ev) ev = event_new(NULL, -1, 0, NULL, NULL);
      event_assign(ev, base, sockfd, EV_WRITE, SocketReadable, ev);
      event_add(ev, NULL);  // add timeout
      io_wait_map_[sockfd] = running_thread_;
      running_thread_->Schedule();
      running_thread_->SetState(Thread::TS_STOP);
      Schedule();

      if (ev->ev_res & EV_TIMEOUT) {
        return -2;
        break;
      } else if (ev->ev_res & EV_WRITE) {
        continue;
      }
      return -1;
    } else if (ret < 0) {
      return -1;
    } else if (0 == ret) {
      return 0;
    }
  }

  return nsend;
}

int Frame::recv(int sockfd, void *buf, size_t len, int flags) {
  size_t recvd = 0;
  struct event *ev = NULL;
  while (true) {
    int ret = ::recv(sockfd, buf, len, flags);
    if (ret > 0) {
      recvd += ret;
      return recvd;
    } else if (0 == ret) {  // reset by peer
      return 0;
    } else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      if (!ev) ev = event_new(NULL, -1, 0, NULL, NULL);
      event_assign(ev, base, sockfd, EV_READ, SocketReadable, ev);
      event_add(ev, NULL);  // add timeout
      io_wait_map_[sockfd] = running_thread_;
      running_thread_->Schedule();
      running_thread_->SetState(Thread::TS_STOP);
      Schedule();

      if (ev->ev_res & EV_TIMEOUT) {
        return -2;  // return timeout errcode
      } else if (ev->ev_res & EV_READ) {
        continue;
      }
      return -1;
    }
  }
  return -1;
}

int Frame::sendto(int sockfd, const void *buf, size_t len, int flags,
                  const struct sockaddr *dest_addr, socklen_t addrlen) {
  return ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t Frame::recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen) {
  struct event *ev = NULL;
  int ret = 0;
  while (true) {
    if ((ret = ::recvfrom(sockfd, buf, len, 0, (struct sockaddr *)src_addr,
                          addrlen)) < 0) {

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (!ev) ev = event_new(NULL, -1, 0, NULL, NULL);
        event_assign(ev, base, sockfd, EV_READ, SocketReadable, ev);
        event_add(ev, NULL);  // add timeout
        io_wait_map_[sockfd] = running_thread_;
        running_thread_->Schedule();
        running_thread_->SetState(Thread::TS_STOP);
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
      break;
    }
  }

  if (ev) event_free(ev);

  return 0;
}
void Frame::SocketReadable(int fd, short events, void *arg) {
  if (events & EV_TIMEOUT) {
    int ret = event_del(static_cast<event *>(arg));
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
    running_thread_->SetState(Thread::TS_STOP);
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
