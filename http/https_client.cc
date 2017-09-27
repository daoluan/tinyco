#include "https_client.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "frame.h"
#include "http_tool.h"
#include "util/network.h"

namespace tinyco {
#define SSTR(x)                                                              \
  static_cast<std::ostringstream &>((std::ostringstream() << std::dec << x)) \
      .str()
namespace http {
static int mt_write(BIO *h, const char *buf, int num);
static int mt_read(BIO *h, char *buf, int size);
static long mt_ctrl(BIO *h, int cmd, long arg1, void *arg2);
static int mt_new(BIO *h);
static int mt_free(BIO *data);

static BIO_METHOD methods_mt = {BIO_TYPE_SOCKET, "socket_mt", mt_write, mt_read,
                                NULL,            NULL,        mt_ctrl,  mt_new,
                                mt_free,         NULL};

BIO_METHOD *BIO_s_socket_mt(void) { return (&methods_mt); }

BIO *BIO_new_socket_mt(int fd, int close_flag, BIOUserData &bud) {
  BIO *ret;

  ret = BIO_new(BIO_s_socket_mt());
  if (ret == NULL) return (NULL);
  BIO_set_fd(ret, fd, close_flag);
  ret->ptr = &bud;
  return (ret);
}

static int mt_new(BIO *bi) {
  bi->init = 0;
  bi->num = 0;
  bi->ptr = NULL;
  bi->flags = 0;
  return (1);
}

static int mt_free(BIO *a) {
  if (a == NULL) return (0);
  if (a->shutdown) {
    if (a->init) {
      shutdown(a->num, SHUT_RDWR);
      close(a->num);
    }
    a->init = 0;
    a->flags = 0;
  }
  return (1);
}

static int mt_read(BIO *b, char *out, int outl) {
  int ret = 0;

  BIOUserData *bud = static_cast<BIOUserData *>(b->ptr);
  if (out != NULL) {
    errno = 0;
    ret = Frame::recv(b->num, out, outl, 0 /*, bud->ms_read_timeout */);
  }
  return (ret);
}

static int mt_write(BIO *b, const char *in, int inl) {
  int ret = 0;

  BIOUserData *bud = static_cast<BIOUserData *>(b->ptr);
  if (in != NULL) {
    errno = 0;
    ret = Frame::send(b->num, in, inl, 0 /*, bud->ms_write_timeout */);
  }
  return (ret);
}

static long mt_ctrl(BIO *b, int cmd, long num, void *ptr) {
  long ret = 1;
  int *ip;

  switch (cmd) {
    case BIO_C_SET_FD:
      mt_free(b);
      b->num = *((int *)ptr);
      b->shutdown = (int)num;
      b->init = 1;
      break;
    case BIO_C_GET_FD:
      if (b->init) {
        ip = (int *)ptr;
        if (ip != NULL) *ip = b->num;
        ret = b->num;
      } else
        ret = -1;
      break;
    case BIO_CTRL_GET_CLOSE:
      ret = b->shutdown;
      break;
    case BIO_CTRL_SET_CLOSE:
      b->shutdown = (int)num;
      break;
    case BIO_CTRL_DUP:
    case BIO_CTRL_FLUSH:
      ret = 1;
      break;
    default:
      ret = 0;
      break;
  }
  return (ret);
}

SSL_CTX *HttpsClient::ctx_ = NULL;

HttpsClient::HttpsClient() : fd_(-1), bio_(NULL), ssl_(NULL) {}

HttpsClient::~HttpsClient() { Close(); }

int HttpsClient::CreateSocket() {
  int keepalive = 1;
  int keepidle = 20;
  int keepinterval = 3;
  int keepcount = 3;
  int val = 0;

  if ((fd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    strerr_.append("socket error");
    return -1;
  }

  if (network::SetNonBlock(fd_) < 0) {
    strerr_.append("fail to set nonblock");
    return -2;
  }

  if (setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive,
                 sizeof(keepalive)) != 0) {
    strerr_.append("setsockopt SO_KEEPALIVE error");
    close(fd_);
    return -4;
  }

  if (setsockopt(fd_, SOL_TCP, TCP_KEEPIDLE, (void *)&keepidle,
                 sizeof(keepidle)) != 0) {
    strerr_.append("setsockopt TCP_KEEPIDLE error");
    close(fd_);
    return -5;
  }

  if (setsockopt(fd_, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval,
                 sizeof(keepinterval)) != 0) {
    strerr_.append("setsockopt TCP_KEEPINTVL error");
    close(fd_);
    return -6;
  }

  if (setsockopt(fd_, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount,
                 sizeof(keepcount)) != 0) {
    strerr_.append("setsockopt TCP_KEEPCNT error");
    close(fd_);
    return -7;
  }

  return fd_;
}

int HttpsClient::Init(uint32_t ip, uint16_t port, int connect_timeout,
                      int read_timeout, int write_timeout) {
  ClearLog();

  bud_.ms_connect_timeout = connect_timeout;
  bud_.ms_write_timeout = write_timeout;
  bud_.ms_read_timeout = read_timeout;

  struct sockaddr_in addr = {0};
  fd_ = CreateSocket();
  int ret = 0;

  if (fd_ < 0) {
    strerr_.append("CreateSocket error");
    return -1;
  }

  if ((bio_ = BIO_new_socket_mt(fd_, 1, bud_)) == NULL) goto end;
  if (ctx_ == NULL) {
    SSL_library_init();
    if ((ctx_ = SSL_CTX_new(TLSv1_client_method())) == NULL) {
      strerr_.append("SSL_CTX_new error");
      goto end;
    }
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ip;

  strlog_.append("HttpsClient connect to ")
      .append(inet_ntoa(addr.sin_addr))
      .append(":")
      .append(SSTR(port));

  if (Frame::connect(fd_, (struct sockaddr *)&addr, sizeof(addr)
                     /*, bud_.ms_connect_timeout */) < 0) {
    strerr_.append("tinyco::connect error: ").append("strerror(errno)");
    goto end;
  }

  if ((ssl_ = SSL_new(ctx_)) == NULL) {
    strerr_.append("SSL_new error");
    goto end;
  }

  SSL_set_bio(ssl_, bio_, bio_);

  if ((ret = SSL_connect(ssl_)) <= 0) {
    strerr_.append("SSL_connect error, error code = ")
        .append(SSTR(SSL_get_error(ssl_, ret)));
    goto end;
  }

  return 0;

end:
  Close();
  return -2;
}

int HttpsClient::SendAndReceive(const char *sendbuf, size_t sendlen,
                                std::string *recvbuf) {
  ClearLog();

  if (fd_ < 0 || NULL == ssl_ || NULL == bio_) {
    strerr_.append("invalid parameters");
    return -1;
  }

  if (NULL == sendbuf || 0 == sendlen || NULL == recvbuf) {
    strerr_.append("invalid parameters");
    return -1;
  }

  return SslSendAndReceive(sendbuf, sendlen, recvbuf);
}

int HttpsClient::SslSendAndReceive(const char *sendbuf, size_t sendlen,
                                   std::string *recvbuf) {
  char recvbuf_[max_recvlen] = {0};
  std::vector<char> real_buf;

  size_t recvpos = 0;
  bool http_done = false;
  int ret = 0;

  size_t left = sendlen;
  while (left) {
    // <=0 包含两种情况：
    // 1. <0 表示发送错误
    // 2. =0 表示服务器重启或宕机了
    if ((ret = SSL_write(ssl_, sendbuf + sendlen - left, left)) <= 0) {
      strerr_.append("SSL_write error, error code = ")
          .append(SSTR(SSL_get_error(ssl_, ret)))
          .append("|ret = ")
          .append(SSTR(ret))
          .append("|errno = ")
          .append(SSTR(errno))
          .append("|strerror = ")
          .append(strerror(errno));
      return -1;
    }

    if ((int)left < ret) {
      strerr_.append("SSL_write error, left = ")
          .append(SSTR(left))
          .append("|ret = ")
          .append(SSTR(ret));
      return -1;
    }

    left -= ret;
  }

  while (true) {
    int recvlen_ = 0;

    // <=0 包含两种情况：
    // 1. <0 表示发送错误
    // 2. =0 表示服务器重启或宕机了
    recvlen_ = SSL_read(ssl_, recvbuf_, (int)max_recvlen);
    if (recvlen_ <= 0) {
      strerr_.append("SSL_read error, error code = ")
          .append(SSTR(SSL_get_error(ssl_, recvlen_)))
          .append("|ret = ")
          .append(SSTR(ret))
          .append("|errno = ")
          .append(SSTR(errno))
          .append("|strerror = ")
          .append(strerror(errno));
      break;
    }

    recvpos += recvlen_;
    real_buf.insert(real_buf.end(), recvbuf_, recvbuf_ + recvlen_);

    // pay attention to oom
    http::HttpParserImpl parser;
    int ret = parser.CheckHttp(&real_buf[0], real_buf.size());
    if (http::HttpParserImpl::HPC_OK == ret) {
      http_done = true;
      // if (status) *status = parser.status_code;
      break;
    } else if (http::HttpParserImpl::HPC_NOT_COMPLETE_HTTP == ret) {
      continue;
    } else if (http::HttpParserImpl::HPC_NOT_HTTP == ret) {
      recvpos = 0;
      break;
    } else {
      recvpos = 0;
      break;
    }
  }

  if (http_done) {
    recvbuf->assign(&real_buf[0], real_buf.size());
    return 0;
  }

  return -1;
}

void HttpsClient::Close() {
  if (ssl_) {
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
    bio_ = NULL;
  }

  if (bio_) {
    BIO_free(bio_);
  }

  if (fd_ != -1) close(fd_);

  fd_ = -1;
  ssl_ = NULL;
  bio_ = NULL;
}
}
}
