#include "tcp_network.h"

#include "frame.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sstream>
#include <string.h>
#include <unistd.h>

using namespace tinyco;

namespace tinyco {
#define SSTR(x)                                                              \
  static_cast<std::ostringstream &>((std::ostringstream() << std::dec << x)) \
      .str()

TcpConn::TcpConn() : fd_(kInvalidFd) {}

TcpConn::~TcpConn() { Close(); }

int TcpConn::Init(uint32_t ip, uint16_t port, int connect_timeout,
                  int read_timeout, int write_timeout) {
  if (0 == ip || 0 == port) return -1;

  ip_ = ip;
  port_ = port;
  connect_timeout_ = connect_timeout;
  read_timeout_ = read_timeout;
  write_timeout_ = write_timeout;

  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == fd_) {
    strlog_.append("|socket error: ").append(strerror(errno));
    return -1;
  }

  if (SetBlocking(false) < 0) {
    Close();
    strlog_.append("|SetBlocking error");
    return -1;
  }

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ip;

  int ret = 0;
  if ((ret = Frame::connect(fd_, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
    Close();
    strlog_.append("|tinyco::connect error: ").append(strerror(errno));
    return -1;
  }

  return 0;
}

int TcpConn::SendAndReceive(const char *buf, size_t size, std::string *output,
                            check_recv_func func) {
  if (NULL == output || NULL == buf || 0 == size) return -1;

  int ret = 0;
  if (kInvalidFd == fd_) {
    strlog_.append("|try to reconnect server");
    if ((ret = TryReConnect()) < 0) {
      strlog_.append("|reconnect server error");
      return -1;
    }
  }

  int left = size;
  while (left > 0) {
    if ((ret = Frame::send(fd_, buf + size - left, left, 0)) < 0) {
      strlog_.append("|tinyco::send error: ").append(strerror(errno));
      return -1;
    }

    if (ret == 0) {
      strlog_.append("|tinyco::send error: server close the connection: ")
          .append(strerror(errno));
      return -1;
    }

    if (ret > left) {
      strlog_.append("|tinyco::send werid error: ").append(strerror(errno));
      return -1;
    }

    left -= ret;
  }

  std::vector<char> real_buf;
  char recvbuf[kMaxRecvSize] = {0};
  int recvd = 0;
  int eof = 0;
  while (true) {
    if ((ret = Frame::recv(fd_, recvbuf, kMaxRecvSize, 0)) < 0) {
      strlog_.append("|tinyco::recv error: ").append(strerror(errno));
      return -1;
    }

    if (ret == 0) {
      strlog_.append("|tinyco::recv error: server close the connection: ")
          .append(strerror(errno))
          .append("|recvd=")
          .append(SSTR(recvd));
      return -1;
    }

    recvd += ret;

    real_buf.insert(real_buf.end(), recvbuf, recvbuf + ret);  // ret > 0

    eof = func(&real_buf[0], real_buf.size());
    if (eof > 0) {
      break;
    } else if (eof < 0 && eof != CHECK_INCOMPLETE) {
      strlog_.append("|recv error data: ret_code=").append(SSTR(eof));
      break;
    }
  }

  if (eof < 0) {
    strlog_.append("|func eof error.");
    return -1;
  }

  output->assign(&real_buf[0], eof);
  return 0;
}

int TcpConn::SendAndReceive(const std::string &buf, std::string *output,
                            check_recv_func func) {
  return SendAndReceive(buf.data(), buf.size(), output, func);
}

int TcpConn::Send(const char *buf, size_t size) {
  int ret = 0;
  if (kInvalidFd == fd_) {
    strlog_.append("|try to reconnect server");
    if ((ret = TryReConnect()) < 0) {
      strlog_.append("|reconnect server error");
      return -1;
    }
  }

  if (NULL == buf || 0 == size) return -1;

  int left = size;
  while (left > 0) {
    if ((ret = Frame::send(fd_, buf + size - left, left, 0)) < 0) {
      strlog_.append("|tinyco::send error: ").append(strerror(errno));
      return -1;
    }

    if (ret == 0) {
      strlog_.append("|tinyco::send error: server close the connection: ")
          .append(strerror(errno));
      return -1;
    }

    if (ret > left) {
      strlog_.append("|tinyco::send werid error: ").append(strerror(errno));
      return -1;
    }

    left -= ret;
  }

  return 0;
}

int TcpConn::Send(const std::string &buf) {
  return Send(buf.data(), buf.size());
}

void TcpConn::Close() {
  if (kInvalidFd != fd_) close(fd_);
  fd_ = kInvalidFd;
  strlog_.clear();
}

int TcpConn::TryReConnect() { return Init(ip_, port_); }

int TcpConn::SetBlocking(bool blocking) {
  int flags;

  if ((flags = fcntl(fd_, F_GETFL)) == -1) {
    strlog_.append("|fcntl GET error");
    return -1;
  }

  if (blocking)
    flags &= ~O_NONBLOCK;
  else
    flags |= O_NONBLOCK;

  if (fcntl(fd_, F_SETFL, flags) == -1) {
    strlog_.append("|fcntl SET error");
    return -1;
  }

  return 0;
}
}
