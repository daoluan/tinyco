#include "frame.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

std::unordered_map<int, std::shared_ptr<Thread>> Frame::thread_map_;
std::list<std::shared_ptr<Thread>> Frame::thread_runnable_;
std::shared_ptr<Thread> Frame::main_thread_;
std::shared_ptr<Thread> Frame::running_thread_;

bool Frame::Init() {
  std::shared_ptr<Thread> m(new Thread);
  m->Init();
  m->SetContext();
  main_thread_ = m;
  running_thread_ = main_thread_;
  return true;
}

int Frame::Process() {
  while (true) {
    int ret = Frame::Schedule();
    sleep(1);
  }
}

int Frame::UdpSendAndRecv(int fd, const std::string &sendbuf, const struct sockaddr_in &dest_addr,
  std::string *recvbuf) {
  int ret = 0;
  char recvbuf_[1024] = { 0 };
  uint32_t recvlen = sizeof(recvbuf_);
  if ((ret = sendto(fd,sendbuf.c_str(),sendbuf.size(),0,(struct sockaddr *)&dest_addr,
    sizeof(dest_addr))) < 0)
  {
    fprintf(stderr, "sendto error\n");
    return -1;
  }

  struct sockaddr_in src_addr;
  socklen_t sockaddr_len = sizeof(src_addr);

  while (true)
  {
    if ((ret = recvfrom(fd,recvbuf_,recvlen,0,(struct sockaddr *)&src_addr,&sockaddr_len)) < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
      {
        Frame::Schedule();
      }
    }
  }

  return 0;
}

int Frame::UdpSendAndRecv(int fd, char *sendbuf, uint32_t senlen, char *recvbuf,
  uint32_t *recvlen) {
  return 0;
}

int Frame::TcpSendAndRecv(int fd, const std::string &sendbuf, std::string *recvbuf,
  IsCompleteBase *is_complete) {
  return 0;
}

int Frame::TcpSendAndRecv(int fd, char *sendbuf, uint32_t senlen, char *recvbuf,
  uint32_t *recvlen, IsCompleteBase *is_complete) {
  return 0;
}

int Frame::Schedule() {
  // assert(thread_runnable_.begin() != thread_runnable_.end());
  // assert(thread_runnable_.size() > 0);
  std::shared_ptr<Thread> pending_thread = Frame::running_thread_;
  thread_runnable_.push_back(pending_thread);

  running_thread_ = *(thread_runnable_.begin());
  thread_runnable_.pop_front();
  // thread_map_.insert({thread->GetThreadId(),thread});

  // if (pending_thread != main_thread_)
  // {
  //   thread_map_.erase(pending_thread->GetThreadId());
  // }

  pending_thread->SwapContext(running_thread_.get());
  return 0;
}
