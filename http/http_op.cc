#include "http_op.h"

#include <sstream>

#include "dns/dns_resolve_impl.h"

namespace tinyco {
namespace http {

int HttpOp::HttpGet(const std::string &url,
                    const std::map<std::string, std::string> &extra_headers,
                    tinyco::http::HttpResponse *hr) {
  return HttpGet(new DNSResolverImpl(), url, extra_headers, hr);
}

int HttpOp::HttpPost(const std::string &url,
                     const std::map<std::string, std::string> &extra_headers,
                     const std::string &content,
                     tinyco::http::HttpResponse *hr) {

  return HttpPost(new DNSResolverImpl(), url, extra_headers, content, hr);
}

int HttpOp::__HttpRequest(uint32_t ip, uint16_t port,
                          const tinyco::http::HttpRequest &http_request,
                          tinyco::http::HttpResponse *http_response) {
  std::string sendbuf, recvbuf;
  http_request.SerializeToString(&sendbuf);

  int ret = EC_NONE;
  if ((ret = __HttpSendAndReveive(ip, port, sendbuf, &recvbuf)) < 0) {
    return ret;
  }

  tinyco::http::HttpParserImpl parser;
  if ((ret = parser.ParseHttpResponse(recvbuf, http_response)) < 0) {
    return EC_RESPONSE_ERROR;
  }

  return ret;
}

int HttpOp::__HttpsRequest(uint32_t ip, uint16_t port,
                           const tinyco::http::HttpRequest &http_request,
                           tinyco::http::HttpResponse *http_response) {
  std::string sendbuf, recvbuf;
  http_request.SerializeToString(&sendbuf);

  int ret = EC_NONE;
  if ((ret = __HttpsSendAndReveive(ip, port, sendbuf, &recvbuf)) < 0) {
    return ret;
  }

  tinyco::http::HttpParserImpl parser;
  if ((ret = parser.ParseHttpResponse(recvbuf, http_response)) < 0) {
    return EC_RESPONSE_ERROR;
  }

  return ret;
}

int HttpOp::__HttpSendAndReveive(uint32_t ip, uint32_t port,
                                 const std::string &sendbuf,
                                 std::string *recvbuf) {
  HttpClient hc;

  int ret = EC_NONE;
  if ((ret = hc.Init(ip, port)) < 0) {
    return EC_INIT_ERROR;
  }

  if ((ret = hc.SendAndReceive(sendbuf.c_str(), sendbuf.size(), recvbuf)) < 0) {
    return EC_SEND_AND_RECEIVE_ERROR;
  }

  return ret;
}

int HttpOp::__HttpsSendAndReveive(uint32_t ip, uint32_t port,
                                  const std::string &sendbuf,
                                  std::string *recvbuf) {
  HttpsClient hc;
  int ret = EC_NONE;
  if ((ret = hc.Init(ip, port)) < 0) {
    return EC_INIT_ERROR;
  }

  if ((ret = hc.SendAndReceive(sendbuf.c_str(), sendbuf.size(), recvbuf)) < 0) {
    return EC_SEND_AND_RECEIVE_ERROR;
  }

  return ret;
}

int HttpOp::HttpGet(DNSResolver *dns_resolver, const std::string &url,
                    const std::map<std::string, std::string> &extra_headers,
                    tinyco::http::HttpResponse *hr) {
  int ret = 0;
  tinyco::http::URL urlobj;
  HttpParserImpl hpi;

  std::string fixed_url;
  if (url.compare(0, sizeof("http://") - 1, "http://") != 0 &&
      url.compare(0, sizeof("https://") - 1, "https://") != 0)
    fixed_url = "http://" + url;
  else
    fixed_url = std::move(url);

  if (hpi.ParseUrl(fixed_url, &urlobj) < 0) {
    return EC_UNKNOWN;
  }

  network::IP ip;
  if (!dns_resolver->Resolve(urlobj.host.c_str(), &ip)) {
    return EC_DNS_RESOLVE_ERROR;
  }

  tinyco::http::HttpRequest http_request;
  http_request.SetHeader("Host", urlobj.host);
  http_request.SetMethod(tinyco::http::HttpRequest::HTTP_REQUEST_METHOD_GET);
  http_request.SetUri(urlobj.uri);

  for (auto &ite : extra_headers) {
    http_request.SetHeader(ite.first, ite.second);
  }

  return (!urlobj.IsHttps())
             ? __HttpRequest(ip.af_inet_ip, urlobj.port, http_request, hr)
             : __HttpsRequest(ip.af_inet_ip, urlobj.port, http_request, hr);
}

int HttpOp::HttpPost(DNSResolver *dns_resolver, const std::string &url,
                     const std::map<std::string, std::string> &extra_headers,
                     const std::string &content,
                     tinyco::http::HttpResponse *hr) {
  int ret = 0;
  tinyco::http::URL urlobj;
  HttpParserImpl hpi;

  std::string fixed_url;
  if (url.compare(0, sizeof("http://") - 1, "http://") != 0 &&
      url.compare(0, sizeof("https://") - 1, "https://") != 0)
    fixed_url = "http://" + url;
  else
    fixed_url = std::move(url);

  if (hpi.ParseUrl(fixed_url, &urlobj) < 0) {
    return EC_UNKNOWN;
  }

  network::IP ip;
  if (!dns_resolver->Resolve(urlobj.host.c_str(), &ip)) {
    return EC_DNS_RESOLVE_ERROR;
  }

  tinyco::http::HttpRequest http_request;
  http_request.SetMethod(tinyco::http::HttpRequest::HTTP_REQUEST_METHOD_POST);
  http_request.SetUri(urlobj.uri);
  http_request.SetHeader("Host", urlobj.host);
  http_request.SetContent(content);

  for (auto &ite : extra_headers) {
    http_request.SetHeader(ite.first, ite.second);
  }

  return (!urlobj.IsHttps())
             ? __HttpRequest(ip.af_inet_ip, urlobj.port, http_request, hr)
             : __HttpsRequest(ip.af_inet_ip, urlobj.port, http_request, hr);
}

int HttpOp::HttpRequest(uint32_t net_ip, uint16_t port,
                        const tinyco::http::HttpRequest &http_request,
                        tinyco::http::HttpResponse *hr) {
  return __HttpRequest(net_ip, port, http_request, hr);
}

int HttpOp::HttpsRequest(uint32_t net_ip, uint16_t port,
                         const tinyco::http::HttpRequest &http_request,
                         tinyco::http::HttpResponse *hr) {
  return __HttpsRequest(net_ip, port, http_request, hr);
}
}
}
