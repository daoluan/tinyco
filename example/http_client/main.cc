#include <assert.h>

#include "server.h"
#include "http/http_client.h"
#include "http/http_request.h"
#include "http/https_client.h"
#include "http/http_op.h"
#include "dns/dns_resolve_impl.h"

using namespace tinyco;
class TestWork : public Work {
 public:
  int Run() {
    network::IP ip;
    dns::DNSResolverImpl dri;
    const std::string &domain = "baidu.com";
    auto bret = dri.Resolve(domain, &ip);
    if (!bret) {
      LOG("dns resolve error, check your dns config");
      return 0;
    }

    LOG("test http...");

    http::HttpClient hc;
    sockaddr_in addr;
    addr.sin_addr.s_addr = ip.af_inet_ip;

    int ret = hc.Init(addr.sin_addr.s_addr, 80);
    if (ret < 0) {
      LOG("ret = %d|%s", ret, hc.GetErrorMsg().c_str());
      return 0;
    }

    http::HttpRequest hr;
    hr.SetMethod(http::HttpRequest::HTTP_REQUEST_METHOD_GET);
    hr.SetUri("/");
    hr.SetHeader("Host", domain);
    std::string req, rsp;
    hr.SerializeToString(&req);

    LOG("http request=%s", req.c_str());
    ret = hc.SendAndReceive(req.c_str(), req.size(), &rsp);
    if (ret < 0) {
      LOG("ret = %d|%s", ret, hc.GetErrorMsg().c_str());
      return 0;
    }
    LOG("http response=%s", rsp.c_str());

    LOG("test https...");
    http::HttpsClient hsc;
    addr.sin_addr.s_addr = ip.af_inet_ip;
    ret = hsc.Init(addr.sin_addr.s_addr, 443);
    if (ret < 0) {
      LOG("ret=%d|%s", ret, hsc.GetErrorMsg().c_str());
      return 0;
    }

    LOG("https request=%s", req.c_str());
    ret = hsc.SendAndReceive(req.c_str(), req.size(), &rsp);
    if (ret < 0) {
      LOG("ret=%d|%s", ret, hsc.GetErrorMsg().c_str());
      return 0;
    }
    LOG("https response = %s", rsp.c_str());

    LOG("test http op");
    std::map<std::string, std::string> extra_headers;
    http::HttpResponse hrsp;
    ret = http::HttpOp::HttpGet("https://www.baidu.com", extra_headers, &hrsp);
    if (ret < 0)
      LOG("HttpGet error: ret=%d", ret);
    else
      LOG("new intf: ret=%d|%s", ret, hrsp.SerializeToString().c_str());
    return 0;
  }
};

int main() {
  assert(Frame::Init());

  auto t = Frame::CreateThread(new TestWork);
  while (!t.IsDead()) {
    Frame::Sleep(1000);
  }

  Frame::Fini();
  return 0;
}
