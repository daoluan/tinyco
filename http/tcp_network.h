#ifndef TCP_NETWORK_H_
#define TCP_NETWORK_H_

#include <inttypes.h>
#include <string>

namespace tinyco {

enum {
  CHECK_ERROR = -1,
  CHECK_INCOMPLETE = -2,
};

class TcpConn {
 public:
  TcpConn();
  virtual ~TcpConn();
  int Init(uint32_t ip, uint16_t port, int connect_timeout = 200,
           int read_timeout = 200, int write_timeout = 200);
  typedef int (*check_recv_func)(void *data, size_t size);
  int SendAndReceive(const char *buf, size_t size, std::string *output,
                     check_recv_func func);
  int SendAndReceive(const std::string &buf, std::string *output,
                     check_recv_func func);
  void Close();

  std::string GetErrorMsg() {
    std::string _e_ = strlog_;
    strlog_.clear();
    return _e_;
  }

  int Send(const char *buf, size_t size);
  int Send(const std::string &buf);

  static const int kInvalidFd = -1;
  static const int kMaxRecvSize = 5120;

 private:
  int TryReConnect();
  int SetBlocking(bool blocking);

 protected:
  uint32_t ip_;
  uint16_t port_;
  int fd_;
  int connect_timeout_;
  int read_timeout_;
  int write_timeout_;

  std::string strlog_;
};
}

#endif
