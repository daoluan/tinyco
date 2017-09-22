#include "server.h"
#include "http/http_server.h"

#include <memory>

using namespace tinyco;

class MyUdpReqWork : public UdpReqWork {
 public:
  int Run() {
    LOG("new udp req: %s|response after 10s", req_.reqpkg.c_str());

    Frame::Sleep(10000);

    LOG("rsp to client");
    Reply(req_.reqpkg);
  }
};

class MyHttpReqWork : public http::HttpSrvWork {
 public:
  virtual int Serve() {
    LOG("ready to reply");
    hrsp_.SetStatus(200);
    hrsp_.SetContent("hello world!");
    Reply();

    return 0;
  }
};

class MyServer : public ServerImpl {
 public:
  virtual TcpReqWork *BuildStreamBusinessWork(uint32_t port) {
    LOG("my work builder: %d", port);
    if (8080 == port) return new MyHttpReqWork();
    return NULL;
  }

  virtual UdpReqWork *BuildUdpBusinessWork(uint32_t port) {
    LOG("my work builder: %d", port);
    if (123456 == port) return new MyUdpReqWork();
    return NULL;
  }
};

int main(int argc, char *argv[]) {
  std::shared_ptr<MyServer> srv(new MyServer);
  if (srv->Initialize(argc, argv) < 0) {
    LOG_ERROR("fail to initialize server");
    return -1;
  }

  srv->Run();

  return 0;
}
