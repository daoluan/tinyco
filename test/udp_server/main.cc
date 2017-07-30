#include <assert.h>

#include "frame.h"

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
  }
};

int main(int argc, char **argv) {
  assert(Frame::Init());
  Frame::UdpSrv<TestWork>(0, 32000);
  Frame::Fini();
  return 0;
}
