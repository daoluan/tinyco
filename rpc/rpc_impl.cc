#include "rpc_impl.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

namespace tinyco {
int TinycoRpc::Serve() {
  auto path = hreq_.GetUriObj().path;
  if (method_map_->find(path) == method_map_->end()) {
    goto reply404;
  }

  // avoid crosses initialization
  {
    const ServerImpl::MethodDescriptor& rpc_md = (*method_map_)[path];
    auto service = rpc_md.service;

    const google::protobuf::ServiceDescriptor* sd = service->GetDescriptor();

    if (sd->method_count() == 0) {
      goto reply404;
    }

    auto method = sd->FindMethodByName(rpc_md.method);
    if (!method) {
      goto reply404;
    }

    LOG_DEBUG("sd fullname=%s|md fullname=%s|path=%s", sd->full_name().c_str(),
              method->full_name().c_str(), hreq_.GetUriObj().path.c_str());

    google::protobuf::Message* request =
        service->GetRequestPrototype(method).New();
    google::protobuf::Message* response =
        service->GetResponsePrototype(method).New();

    LOG_DEBUG("request content size=%d", hreq_.GetContent().size());
    if (!request->ParseFromString(hreq_.GetContent())) {
      return -1;
    }

    service->CallMethod(method, NULL, request, response, NULL);

    std::string strrsp;
    response->SerializeToString(&strrsp);
    hrsp_.SetContent(std::move(strrsp));
    Reply();

    return 0;
  }

reply404:
  hrsp_.SetStatus(404);
  Reply();
  return 0;
}
}
