#include <arpa/inet.h>
#include <assert.h>

#include "frame.h"

using namespace tinyco;
class TestWork : public Work {
 public:
  TestWork() {}
  virtual ~TestWork() {}

  int Run() {
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    addr.sin_port = htons(32000);

    std::string sendbuf = "hello";

    LOG("sleep 1 second");
    Frame::Sleep(1000);
    LOG("sleep 2 second");
    Frame::Sleep(2000);

    LOG("client send pkg to udp server and receive the same content");
    LOG("in order to test, udp server will wait for 1 second after receiving "
        "pkg from client");
    LOG("send content: %s", sendbuf.c_str());
    std::string recvbuf;
    int ret = Frame::UdpSendAndRecv(sendbuf, addr, &recvbuf);
    if (ret < 0) {
      LOG("UdpSendAndRecv error: ret=%d", ret);
      return -1;
    }

    LOG("recv content: %s", recvbuf.c_str());
  }
};

int main() {
  assert(Frame::Init());
  Frame::CreateThread(new TestWork);
  Frame::Schedule();
  Frame::Fini();

  return 0;
}
