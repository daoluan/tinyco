#ifndef HTTP_TOOL_
#define HTTP_TOOL_

#include "http_request.h"
#include "http_response.h"

#include <string>

namespace tinyco {
namespace http {

class HttpParser {
 public:
  enum HttpParseCode {
    HPC_OK = 0,
    HPC_NOT_HTTP = -1,
    HPC_NOT_COMPLETE_HTTP = -2,
    HPC_UNKNOWN,
  };

  HttpParser() {}

  virtual ~HttpParser() {}

  // 解析 http 请求
  // 成功返回 0；失败返回 -1
  virtual int ParseHttpRequest(const std::string &httpbuf,
                               HttpRequest *http_request) = 0;

  virtual int ParseHttpRequestHeadOnly(const std::string &httpbuf,
                                       HttpRequest *http_request,
                                       uint32_t *nparsed) = 0;

  // 解析 http 响应
  // 成功返回 0；失败返回 -1
  virtual int ParseHttpResponse(const std::string &httpbuf,
                                HttpResponse *http_response) = 0;

  virtual int CheckHttp(const std::string &httpbuf) = 0;
  virtual int CheckHttp(const std::string &httpbuf, size_t *szparsed) = 0;
  virtual int CheckHttp(const void *httpbuf, size_t size) = 0;
  virtual int CheckHttp(const void *httpbuf, size_t size, size_t *szparsed) = 0;

 private:
  HttpParser(const HttpParser &);
  void operator=(const HttpParser &);
};

class HttpParserImpl : public HttpParser {
 public:
  HttpParserImpl() {}
  virtual ~HttpParserImpl() {}

  // 解析 http 请求
  // 成功返回 0；失败返回 -1
  virtual int ParseHttpRequest(const std::string &httpbuf,
                               HttpRequest *http_request);

  virtual int ParseHttpRequestHeadOnly(const std::string &httpbuf,
                                       HttpRequest *http_request,
                                       uint32_t *nparsed);

  // 解析 http 响应
  // 成功返回 0；失败返回 -1
  virtual int ParseHttpResponse(const std::string &httpbuf,
                                HttpResponse *http_response);

  virtual int CheckHttp(const std::string &httpbuf);
  virtual int CheckHttp(const std::string &httpbuf, size_t *szparsed);
  virtual int CheckHttp(const void *httpbuf, size_t size);
  virtual int CheckHttp(const void *httpbuf, size_t size, size_t *szparsed);
};
}
}

#endif
