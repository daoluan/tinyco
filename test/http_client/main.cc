#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#include "frame.h"
#include "http_client.h"
#include "http_request.h"
#include "https_client.h"

using namespace tinyco;
class TestWork : public Work {
 public:
  TestWork() {}
  virtual ~TestWork() {}

  int Run() {
    LOG("test http...");
    http::HttpClient hc;
    sockaddr_in addr;
    inet_aton("220.181.57.217", &addr.sin_addr);

    int ret = hc.Init(addr.sin_addr.s_addr, 80);
    if (ret < 0) {
      LOG("ret = %d|%s", ret, hc.GetErrorMsg().c_str());
      return 0;
    }

    http::HttpRequest hr;
    hr.SetMethod(http::HttpRequest::HTTP_REQUEST_METHOD_GET);
    hr.SetUri("/");
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
    inet_aton("220.181.57.217", &addr.sin_addr);
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

    return 0;
  }
};

int main() {
  tinyco::Frame::Init();
  Frame::CreateThread(new TestWork);
  Frame::Schedule();
  Frame::Fini();
  return 0;
}
