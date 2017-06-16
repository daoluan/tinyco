#include <assert.h>

#include "http_server.h"

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

int main() {
  assert(Frame::Init());
  Frame::TcpSrv<TestWork>(0, 8080);
  Frame::Fini();
  return 0;
}
