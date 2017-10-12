#include <assert.h>

#include "server.h"
#include "rpc/rpc_impl.h"
#include "service.pb.h"

using namespace tinyco;

// implement your rpc service's business
class HttpServiceImpl : public Service {
 public:
  virtual void Foo(::google::protobuf::RpcController* controller,
                   const ::MyRequest* request, ::MyResponse* response,
                   ::google::protobuf::Closure* done) {
    response->set_content("world");
  }
};

class MyServer : public ServerImpl {
 public:
  virtual TcpReqWork* BuildStreamBusinessWork(uint32_t port) {
    LOG("my work builder: %d", port);
    // when new requet comes to the port, new rpc object with method map
    if (8080 == port) return new tinyco::TinycoRpc(method_map_);
    return NULL;
  }

  virtual UdpReqWork* BuildUdpBusinessWork(uint32_t port) { return NULL; }
};

int main(int argc, char* argv[]) {
  std::shared_ptr<MyServer> srv(new MyServer);
  if (srv->Initialize(argc, argv) < 0) {
    LOG_ERROR("fail to initialize server");
    return -1;
  }

  // install service
  if (!srv->AddRpcService(new HttpServiceImpl(),
                          "/v1/queue/start  > Foo,"
                          "/v1/queue/stop   > Foo,"
                          "/v1/queue/stats/ > Foo")) {
    LOG_ERROR("fail to add rpc service");
    return -1;
  }

  srv->Run();

  return 0;
}
