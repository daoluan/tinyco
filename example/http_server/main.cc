#include <assert.h>

#include "http/http_server.h"

using namespace tinyco;

class TestWork : public http::HttpSrvWork {
 public:
  virtual int Serve() {
    LOG("ready to reply");
    hrsp_.SetStatus(200);
    hrsp_.SetContent("hello world!");
    Reply();

    return 0;
  }
};

class MyBuilder : public BusinessWorkBuilder {
 public:
  virtual TcpReqWork *BuildStreamBusinessWork(uint32_t port) {
    return new TestWork();
  }
  virtual UdpReqWork *BuildUdpBusinessWork(uint32_t port) { return NULL; }
};

int main() {
  TcpSrv(0, 8080, new MyBuilder());
  return 0;
}
