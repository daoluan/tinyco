#include "http_server.h"

#include <string>

#include "frame.h"
#include "http_tool.h"

namespace tinyco {
namespace http {

class HttpCheckPkg : public IsCompleteBase {
 public:
  virtual int CheckPkg(const char* buffer, uint32_t buffer_len) {
    HttpParserImpl hpi;
    size_t real_len = 0;
    int ret = hpi.CheckHttp(buffer, buffer_len, &real_len);
    if (HttpParserImpl::HPC_OK == ret)
      return real_len;
    else if (HttpParserImpl::HPC_NOT_COMPLETE_HTTP == ret)
      return 0;
    else if (HttpParserImpl::HPC_NOT_HTTP == ret)
      return -1;
    return -1;
  }
};

int HttpSrvWork::Run() {
  std::string req;
  req.resize(512);
  size_t recvd = 0;

  HttpCheckPkg hcp;

  while (true) {
    // grow up buffer size
    if (recvd == req.size()) {
      req.resize(req.size() * 2);
    }

    int ret = Frame::recv(sockfd_, const_cast<char*>(req.data()) + recvd,
                          req.size() - recvd, 0);
    if (ret > 0) {
      recvd += ret;

      // check pkg
      int nparsed = hcp.CheckPkg(req.data(), recvd);
      if (nparsed > 0) {
        // process pkg
        HttpParserImpl hpi;
        ret = hpi.ParseHttpRequest(req.substr(0, nparsed), &hreq_);

        if (0 == ret) {
          Serve();

          // move left request to front
          if (recvd > nparsed) {
            req = req.substr(nparsed);
          } else if (recvd == nparsed) {
            recvd = 0;
          }

          if (hreq_.KeepAlive()) {
            continue;
          }
          break;

        } else {  // parse error
          LOG("fail to parse request: %d", ret);
          return -__LINE__;
        }
      } else if (0 == nparsed) {  // continue to recv
        continue;
      } else if (nparsed < 0) {  // not http
        return -__LINE__;
      }
    } else if (0 == ret)  // close by peer
      return -__LINE__;
    else if (ret < 0) {  // sys error!
      return -__LINE__;
    }
  }

  return 0;
}

int HttpSrvWork::Reply() {
  std::string rsp;
  if (hreq_.IsHeader("Connection"))
    hrsp_.SetHeader("Connection", hreq_.GetHeader("Connection"));
  hrsp_.SerializeToString(&rsp);

  return Frame::send(sockfd_, rsp.data(), rsp.size(), 0);
}
}
}
