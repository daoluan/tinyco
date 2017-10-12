#include <assert.h>

#include "server.h"
#include "rpc/controller.h"
#include "rpc/channel.h"
#include "service.pb.h"
#include "util/log.h"

using namespace tinyco;

class TestWork : public Work {
 public:
  int Run() {
    Channel channel;
    Controller ctrl;

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    channel.InitServer(
        network::EndPoint(network::IP{addr.sin_addr.s_addr}, 8080));

    ctrl.set_connect_timoeut(1000);
    ctrl.set_recv_timoeut(2000);
    ctrl.http_requeset_header().SetUri("/v1/queue/stop");

    MyRequest request;
    request.set_content("hello");
    MyResponse response;

    channel.CallMethod(NULL, &ctrl, &request, &response, NULL);

    LOG("CallMethod ret=%d|response=%s", ctrl.error_code(),
        response.DebugString().c_str());
    return 0;
  }
};

int main(int argc, char* argv[]) {
  assert(Frame::Init());
  Frame::CreateThread(new TestWork);
  Frame::Schedule();
  Frame::Fini();
  return 0;
}
