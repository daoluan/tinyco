#ifndef RPC_IMPL_H_
#define RPC_IMPL_H_

#include "http/http_server.h"

#include <memory>
#include <google/protobuf/service.h>

namespace tinyco {
class TinycoRpc : public http::HttpSrvWork {
 public:
  TinycoRpc() = delete;
  TinycoRpc(std::unordered_map<std::string, ServerImpl::MethodDescriptor> &
                method_map) {
    method_map_ = &method_map;
  }

  int Serve();

 private:
  std::unordered_map<std::string, ServerImpl::MethodDescriptor> *method_map_;
};
}

#endif
