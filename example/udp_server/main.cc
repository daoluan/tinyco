#include <assert.h>

#include "server.h"

using namespace tinyco;

class TestWork : public UdpReqWork {
 public:
  TestWork() {}
  virtual ~TestWork() {}

  int Run() {
    LOG("new udp req: %s|response after 10s", req_.reqpkg.c_str());

    Frame::Sleep(10000);

    LOG("rsp to client");
    Reply(req_.reqpkg);

    return 0;
  }
};

class MyBuilder : public BusinessWorkBuilder {
 public:
  virtual UdpReqWork *BuildUdpBusinessWork(uint32_t port) {
    return new TestWork();
  }
  virtual TcpReqWork *BuildStreamBusinessWork(uint32_t port) { return NULL; }
};

int main() {
  UdpSrv(0, 32000, new MyBuilder());
  return 0;
}
