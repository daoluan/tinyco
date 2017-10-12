
#include "channel.h"

#include <google/protobuf/message.h>

#include "controller.h"
#include "http/http_client.h"
#include "http/http_op.h"
#include "util/log.h"

namespace tinyco {
void Channel::CallMethod(const google::protobuf::MethodDescriptor* method,
                         google::protobuf::RpcController* controller,
                         const google::protobuf::Message* request,
                         google::protobuf::Message* response,
                         google::protobuf::Closure* done) {

  Controller* cntl = reinterpret_cast<Controller*>(controller);

  http::HttpRequest hreq = cntl->http_requeset_header();
  http::HttpResponse hrsp;
  std::string req_content;
  request->SerializeToString(&req_content);

  hreq.SetContent(req_content);

  LOG("hreq=%s", hreq.SerializeToString().c_str());

  if (hrsp.GetStatus() != 200) {
    // TODO copy header only
    return;
  }

  int ret = http::HttpOp::HttpRequest(server_.ip.af_inet_ip, server_.port, hreq,
                                      &hrsp);
  if (ret < 0) {
    cntl->set_error_code(-1);
    return;
  }

  if (!response->ParseFromString(hrsp.GetContent())) {
    LOG_ERROR("ParseFromString error");
    return;
  }
}
}
