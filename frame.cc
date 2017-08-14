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
#include <memory>

#include "util/time.h"

namespace tinyco {

std::unordered_map<int, Thread *> Frame::io_wait_map_;  // wait on io
std::list<Thread *> Frame::thread_runnable_;
std::list<Thread *> Frame::thread_free_;       // like memory pool
std::vector<Thread *> Frame::thread_pending_;  // sleeping thread
Thread *Frame::main_thread_ = NULL;
Thread *Frame::running_thread_ = NULL;
struct event_base *Frame::base_ = NULL;
uint64_t Frame::last_loop_ts_ = 0;

struct ThreadPendingTimeComp {
  bool operator()(Thread *&a, Thread *&b) const {
    return a->GetWakeupTime() > b->GetWakeupTime();
  }
};

bool Frame::Init() {
  // avoid duplicate initialization
  if (main_thread_ || base_) return false;

  auto m = new Thread;
  m->Init();
  m->SetContext(MainThreadLoop, NULL);
  main_thread_ = m;

  base_ = NULL;
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

  for (auto it = io_wait_map_.begin(); it != io_wait_map_.end();) {
    delete (((it++)->second));
  }

  if (base_) event_base_free(base_);
  return true;
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
    while (sz-- && n->GetWakeupTime() <= nowms) {
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
    if (mint->GetWakeupTime() < nowms) {
      timeout.tv_usec = 0;
      return timeout;
    }

    int delta = mint->GetWakeupTime() - nowms;
    timeout.tv_sec = delta / 1000llu;
    timeout.tv_usec = (delta - timeout.tv_sec * 1000llu) * 1000llu;
  }
  return timeout;
}

int Frame::EventLoop(const timeval &tv) {
  event_base_loopexit(GetEventBase(), &tv);
  return event_base_loop(GetEventBase(), EVLOOP_ONCE);
}

int Frame::UdpSendAndRecv(const std::string &sendbuf,
                          struct sockaddr_in &dest_addr, std::string *recvbuf) {
  int ret = 0;
  char recvbuf_[65536] = {0};
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
  if ((ret = recvfrom(fd, recvbuf_, sizeof(recvbuf_), 0,
                      (struct sockaddr *)&dest_addr, &sockaddr_len)) < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return -2;
    else
      return -2;
  }

  if (ret > 0) {
    recvbuf->assign(recvbuf_, ret);
  }

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

int Frame::accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int fd = -1;
  struct event ev;
  while (fd < 0) {
    fd = ::accept(sockfd, addr, addrlen);
    if (fd < 0) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK) {

        event_assign(&ev, GetEventBase(), sockfd, EV_READ, SocketReadOrWrite,
                     &ev);
        event_add(&ev, NULL);
        io_wait_map_[sockfd] = running_thread_;
        running_thread_->Schedule();
        running_thread_->SetState(Thread::TS_STOP);
        Schedule();
      } else {
        return -1;
      }
    }
  }

  return fd;
}

int Frame::connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  struct event ev;
  int ret = 0;
  while ((ret = ::connect(sockfd, addr, addrlen)) < 0) {
    if (errno == EINTR) continue;
    if (EINPROGRESS == errno || EADDRINUSE == errno) {
      event_assign(&ev, GetEventBase(), sockfd, EV_WRITE, SocketReadOrWrite,
                   &ev);
      event_add(&ev, NULL);  // add timeout
      io_wait_map_[sockfd] = running_thread_;
      running_thread_->Schedule();
      running_thread_->SetState(Thread::TS_STOP);
      Schedule();

      if (ev.ev_res & EV_TIMEOUT) {
        return -1;
      } else if (ev.ev_res != EV_WRITE) {
        return -1;
      }

      int err = 0, n = 0;
      if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *)&err,
                     (socklen_t *)&n) < 0) {
        return -1;
      }

      if (err) {
        errno = err;
        return -1;
      }

      break;
    } else {
      return -1;
    }
  }

  return 0;
}

int Frame::send(int sockfd, const void *buf, size_t len, int flags) {
  struct event ev;
  size_t nsend = 0;
  while (nsend < len) {
    int ret =
        ::send(sockfd, static_cast<const char *>(buf) + nsend, len - nsend, 0);
    if (ret > 0) {
      nsend += ret;
    }
    if (ret < 0 && errno == EINTR) {
      continue;
    } else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      event_assign(&ev, GetEventBase(), sockfd, EV_WRITE, SocketReadOrWrite,
                   &ev);
      event_add(&ev, NULL);
      io_wait_map_[sockfd] = running_thread_;
      running_thread_->Schedule();
      running_thread_->SetState(Thread::TS_STOP);
      Schedule();

      if (ev.ev_res & EV_TIMEOUT) {
        return -2;
        break;
      } else if (ev.ev_res & EV_WRITE) {
        continue;
      }
      return -1;
    } else if (ret < 0) {
      return -1;
    } else if (0 == ret) {
      return 0;  // close by peer
    }
  }

  return nsend;
}

int Frame::recv(int sockfd, void *buf, size_t len, int flags) {
  size_t recvd = 0;
  struct event ev;
  int ret = 0;
  while (true) {
    ret = ::recv(sockfd, buf, len, flags);
    if (ret > 0) {
      recvd += ret;
      return recvd;
    } else if (0 == ret) {  // reset by peer
      return 0;
    } else if (ret < 0) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        event_assign(&ev, GetEventBase(), sockfd, EV_READ | EV_CLOSED,
                     SocketReadOrWrite, &ev);
        event_add(&ev, NULL);
        io_wait_map_[sockfd] = running_thread_;
        running_thread_->Schedule();
        running_thread_->SetState(Thread::TS_STOP);
        Schedule();

        if (ev.ev_res & EV_TIMEOUT) {
          return -2;  // return timeout errcode
        } else if (ev.ev_res & EV_CLOSED) {
          return 0;
        } else if (ev.ev_res & EV_READ) {
          continue;
        }
      }

      break;
    }
  }

  return ret;
}

int Frame::sendto(int sockfd, const void *buf, size_t len, int flags,
                  const struct sockaddr *dest_addr, socklen_t addrlen) {
  return ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t Frame::recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen) {
  struct event ev;
  int ret = 0;
  while (true) {
    int ret =
        ::recvfrom(sockfd, buf, len, 0, (struct sockaddr *)src_addr, addrlen);
    if (ret > 0) {
      return ret;
    } else if (0 == ret) {
      return 0;
    } else if (ret < 0) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        event_assign(&ev, GetEventBase(), sockfd, EV_READ | EV_CLOSED,
                     SocketReadOrWrite, &ev);
        event_add(&ev, NULL);
        io_wait_map_[sockfd] = running_thread_;
        running_thread_->Schedule();
        running_thread_->SetState(Thread::TS_STOP);
        Schedule();

        if (ev.ev_res & EV_TIMEOUT) {
          return -2;  // return timeout errcode
        } else if (ev.ev_res & EV_CLOSED) {
          return 0;
        } else if (ev.ev_res & EV_READ) {
          continue;
        }
      }

      break;
    }
  }

  return ret;
}

void Frame::SocketReadOrWrite(int fd, short events, void *arg) {
  int ret = event_del(static_cast<event *>(arg));

  thread_runnable_.push_back(io_wait_map_[fd]);
  io_wait_map_.erase(fd);
  event_base_loopbreak(Frame::base_);
}

int HandleProcess(void *arg) {
  auto *w = static_cast<Work *>(arg);
  LOG_DEBUG("run ret=%d", w->Run());
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
  // work will be deleted by HandleProcess()
  // thread should not be deleted before deleting work
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
    running_thread_->Pending(time::mstime() + ms);
    running_thread_->SetState(Thread::TS_STOP);
    PendThread(running_thread_);
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

void Frame::UpdateLoopTimestamp() { last_loop_ts_ = time::mstime(); }
}
