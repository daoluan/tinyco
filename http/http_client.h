#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

#include "tcp_conn.h"

namespace tinyco {
namespace http {

// HttpClient hc;
// hc.Init(ip, port);
// hc.SendAndReceive(...);

class HttpClient : public TcpConn {
 public:
  HttpClient();
  virtual ~HttpClient();

  std::string GetErrorMsg() {
    std::string _e_ = strerr_;
    strerr_.clear();
    return _e_;
  }

  int Init(uint32_t ip, uint16_t port);
  int SendAndReceive(const char *sendbuf, size_t sendlen, uint32_t *status,
                     char *recvbuf, size_t *recvlen);
  int SendAndReceive(const char *sendbuf, size_t sendlen, std::string *recvbuf);
  int SendAndReceive(const char *sendbuf, size_t sendlen, char *recvbuf,
                     size_t *recvlen);

  void SetConnectMsTimeout(int timeout) {
    if (timeout < 0)
      ms_connect_timeout_ = kMsConnectTimeout;
    else
      ms_connect_timeout_ = timeout;
  }

  void SetReadMsTimeout(int timeout) {
    if (timeout < 0)
      ms_read_timeout_ = kMsReadTimeout;
    else
      ms_read_timeout_ = timeout;
  }

  void SetWriteMsTimeout(int timeout) {
    if (timeout < 0)
      ms_write_timeout_ = kMsWriteTimeout;
    else
      ms_write_timeout_ = timeout;
  }

 private:
  static int HttpCheckComplete(void *data, size_t size);

 public:
  static const int kMsConnectTimeout = 1000;
  static const int kMsReadTimeout = 5000;
  static const int kMsWriteTimeout = 500;

 private:
  std::string strerr_;

  int ms_connect_timeout_;
  int ms_read_timeout_;
  int ms_write_timeout_;
};
}
}

#endif
