#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <google/protobuf/service.h>

#include "util/network.h"

namespace tinyco {
class Channel : public google::protobuf::RpcChannel {
 public:
  bool InitServer(const network::EndPoint& server) { server_ = server; }
  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done);

 private:
  network::EndPoint server_;
};
}

#endif
