#ifndef HTTPS_CLIENT_H_
#define HTTPS_CLIENT_H_

#include <inttypes.h>
#include <string>

#include "openssl/bio.h"
#include "openssl/ssl.h"

// HttpsClient hc;
// hc.Init(myip);
// hc.SendAndRecv(...);

namespace tinyco {
namespace http {

struct BIOUserData {
  int ms_connect_timeout;
  int ms_read_timeout;
  int ms_write_timeout;
};

class HttpsClient {
 public:
  HttpsClient();

  ~HttpsClient();

  int Init(uint32_t ip, uint16_t port = 443,
           int connect_timeout = kMsBIOConnectTimeout,
           int read_timeout = kMsBIOReadTimeout,
           int write_timeout = kMsBIOWriteTimeout);
  int SendAndReceive(const char *sendbuf, size_t sendlen, std::string *recvbuf);
  void Close();

  std::string GetErrorMsg() {
    std::string _e_ = strerr_;
    strerr_.clear();
    return _e_;
  }

  const static int kMsBIOConnectTimeout = 1000;
  const static int kMsBIOReadTimeout = 5000;
  const static int kMsBIOWriteTimeout = 500;

 private:
  int CreateSocket();
  int SslSendAndReceive(const char *sendbuf, size_t sendlen,
                        std::string *recvbuf);

  void ClearLog() {
    strlog_.clear();
    strerr_.clear();
  }

 private:
  int fd_;
  static SSL_CTX *ctx_;
  BIO *bio_;
  SSL *ssl_;

  std::string strlog_;
  std::string strerr_;
  BIOUserData bud_;

  static const size_t max_recvlen = 5120;
};
}
}

#endif
