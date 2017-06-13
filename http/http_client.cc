#include "http_client.h"

#include <arpa/inet.h>
#include <string.h>

#include "tcp_network.h"
#include "http_parser.h"
#include "http_tool.h"

namespace tinyco {
namespace http {

HttpClient::HttpClient()
    : ms_connect_timeout_(kMsConnectTimeout),
      ms_read_timeout_(kMsReadTimeout),
      ms_write_timeout_(kMsWriteTimeout) {}

HttpClient::~HttpClient() { Close(); }

int HttpClient::Init(uint32_t ip, uint16_t port) {
  if (0 == ip || 0 == port) {
    return -1;
  }

  int ret = 0;
  if ((ret = this->TcpConn::Init(ip, port, ms_connect_timeout_,
                                 ms_read_timeout_, ms_write_timeout_)) < 0) {
    return -2;
  }

  return 0;
}

int HttpClient::SendAndReceive(const char *sendbuf, size_t sendlen,
                               char *recvbuf, size_t *recvlen) {
  if (NULL == sendbuf || 0 == sendlen || NULL == recvbuf || NULL == recvlen) {
    return -1;
  }

  std::string str_recvbuf;
  int ret = this->TcpConn::SendAndReceive(sendbuf, sendlen, &str_recvbuf,
                                          HttpCheckComplete);
  if (ret < 0) {
    return -2;
  }
  if (*recvlen < str_recvbuf.size()) return -3;

  memcpy(recvbuf, str_recvbuf.c_str(), str_recvbuf.size());
  *recvlen = str_recvbuf.size();
  return ret;
}

int HttpClient::SendAndReceive(const char *sendbuf, size_t sendlen,
                               std::string *recvbuf) {
  if (NULL == recvbuf) return -1;
  return this->TcpConn::SendAndReceive(sendbuf, sendlen, recvbuf,
                                       HttpCheckComplete);
}

int HttpClient::HttpCheckComplete(void *data, size_t size) {
  tinyco::http::HttpRequest http_request;
  std::string httpbuf = std::string(static_cast<char *>(data), size);
  tinyco::http::HttpParserImpl parser;
  int ret = parser.CheckHttp(httpbuf);
  if (ret == tinyco::http::HttpParserImpl::HPC_OK)
    return size;
  else if (ret ==
           http::HttpParserImpl::HPC_NOT_HTTP)  // not http, clear recvbuf_
    return 0;
  else if (ret == http::HttpParserImpl::HPC_NOT_COMPLETE_HTTP)
    return CHECK_INCOMPLETE;
  else
    return CHECK_ERROR;
}
}
}
