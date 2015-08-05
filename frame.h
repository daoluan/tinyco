#ifndef FRAME_H__
#define FRAME_H__

#include <inttypes.h>
#include <unordered_map>
#include <list>
#include <memory>
#include "thread.h"

class IsCompleteBase {
public:
  IsCompleteBase () {}
  virtual ~IsCompleteBase () {}

  bool operator()(char *buffer, uint32_t buffer_len) {
    return true;
  }
};

class Frame {
public:
  Frame () {}
  static int Process();
  virtual ~Frame () {}

  static bool Init();
public:
  static int UdpSendAndRecv(int fd, const std::string &sendbuf, const struct sockaddr_in &dest_addr,
    std::string *recvbuf);
  static int UdpSendAndRecv(int fd, char *sendbuf, uint32_t senlen, char *recvbuf,
    uint32_t *recvlen);
  static int TcpSendAndRecv(int fd, const std::string &sendbuf, std::string *recvbuf,
    IsCompleteBase *is_complete);
  static int TcpSendAndRecv(int fd, char *sendbuf, uint32_t senlen, char *recvbuf,
    uint32_t *recvlen, IsCompleteBase *is_complete);

  static void RecordRunnableThread() {
  }

  static int Schedule();

public:
  // TODO 如果不是 fd 怎么办，不能完全按网络的思路来写
  static std::unordered_map<int, std::shared_ptr<Thread>> thread_map_;
  static std::list<std::shared_ptr<Thread>> thread_runnable_;
  static std::shared_ptr<Thread> main_thread_;
  static std::shared_ptr<Thread> running_thread_;
};

#endif
