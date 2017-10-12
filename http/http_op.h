#ifndef HTTP_OP_H_
#define HTTP_OP_H_

#include <map>
#include <tr1/unordered_map>

#include "http_tool.h"
#include "http_client.h"
#include "https_client.h"
#include "http_request.h"
#include "http_response.h"
#include "dns/dns_resolve.h"

using namespace tinyco::dns;

namespace tinyco {
namespace http {

class HttpRequest;
class HttpResponse;

class HttpOp {
 public:
  enum ErrorCode {
    EC_NONE = 0,
    EC_DNS_RESOLVE_ERROR = -1,
    EC_INIT_ERROR = -2,
    EC_SEND_AND_RECEIVE_ERROR = -3,
    EC_RESPONSE_ERROR = -4,
    EC_UNKNOWN = -5,
  };

  // interface without details
  static int HttpGet(const std::string &url,
                     const std::map<std::string, std::string> &extra_headers,
                     tinyco::http::HttpResponse *hr);

  static int HttpPost(const std::string &url,
                      const std::map<std::string, std::string> &extra_headers,
                      const std::string &content,
                      tinyco::http::HttpResponse *hr);
  ////

  static int HttpGet(DNSResolver *dns_resolver, const std::string &url,
                     const std::map<std::string, std::string> &extra_headers,
                     tinyco::http::HttpResponse *hr);

  static int HttpPost(DNSResolver *dns_resolver, const std::string &url,
                      const std::map<std::string, std::string> &extra_headers,
                      const std::string &content,
                      tinyco::http::HttpResponse *hr);

  static int HttpRequest(uint32_t net_ip, uint16_t port,
                         const tinyco::http::HttpRequest &http_request,
                         tinyco::http::HttpResponse *hr);

  static int HttpsRequest(uint32_t net_ip, uint16_t port,
                          const tinyco::http::HttpRequest &http_request,
                          tinyco::http::HttpResponse *hr);

 private:
  static int __HttpRequest(uint32_t ip, uint16_t port,
                           const tinyco::http::HttpRequest &http_request,
                           tinyco::http::HttpResponse *http_response);

  static int __HttpSendAndReveive(uint32_t ip, uint32_t port,
                                  const std::string &sendbuf,
                                  std::string *recvbuf);

  static int __HttpsRequest(uint32_t ip, uint16_t port,
                            const tinyco::http::HttpRequest &http_request,
                            tinyco::http::HttpResponse *http_response);

  static int __HttpsSendAndReveive(uint32_t ip, uint32_t port,
                                   const std::string &sendbuf,
                                   std::string *recvbuf);
};
}
}

#endif
