#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <google/protobuf/service.h>

#include "util/network.h"
#include "http/http_response.h"
#include "http/http_request.h"

namespace tinyco {
class Controller : public google::protobuf::RpcController {
 public:
  Controller() : error_code_(0) {
    hreq_header_.SetMethod(http::HttpRequest::HTTP_REQUEST_METHOD_POST);
  }
  int connect_timeout() const { return connect_timeout_; }
  void set_connect_timoeut(int timeout) { connect_timeout_ = timeout; }

  int recv_timeout() const { return recv_timeout_; }
  void set_recv_timoeut(int timeout) { recv_timeout_ = timeout; }

  int error_code() const { return error_code_; }
  void set_error_code(int ec) { error_code_ = ec; }

  http::HttpResponse& http_response_header() { return hrsp_header_; }
  http::HttpRequest& http_requeset_header() { return hreq_header_; }

  virtual void Reset() {}

  // After a call has finished, returns true if the call failed.  The possible
  // reasons for failure depend on the RPC implementation.  Failed() must not
  // be called before a call has finished.  If Failed() returns true, the
  // contents of the response message are undefined.
  virtual bool Failed() const { return false; }

  // If Failed() is true, returns a human-readable description of the error.
  virtual std::string ErrorText() const { return ""; }

  // Advises the RPC system that the caller desires that the RPC call be
  // canceled.  The RPC system may cancel it immediately, may wait awhile and
  // then cancel it, or may not even cancel the call at all.  If the call is
  // canceled, the "done" callback will still be called and the RpcController
  // will indicate that the call failed at that time.
  virtual void StartCancel() {}

  // Server-side methods ---------------------------------------------
  // These calls may be made from the server side only.  Their results
  // are undefined on the client side (may crash).

  // Causes Failed() to return true on the client side.  "reason" will be
  // incorporated into the message returned by ErrorText().  If you find
  // you need to return machine-readable information about failures, you
  // should incorporate it into your response protocol buffer and should
  // NOT call SetFailed().
  virtual void SetFailed(const std::string& reason) {}

  // If true, indicates that the client canceled the RPC, so the server may
  // as well give up on replying to it.  The server should still call the
  // final "done" callback.
  virtual bool IsCanceled() const { return false; }

  // Asks that the given callback be called when the RPC is canceled.  The
  // callback will always be called exactly once.  If the RPC completes without
  // being canceled, the callback will be called after completion.  If the RPC
  // has already been canceled when NotifyOnCancel() is called, the callback
  // will be called immediately.
  //
  // NotifyOnCancel() must be called no more than once per request.
  virtual void NotifyOnCancel(google::protobuf::Closure* callback) {}

 private:
  int connect_timeout_;
  int recv_timeout_;
  int error_code_;

  http::HttpRequest hreq_header_;
  http::HttpResponse hrsp_header_;
};
}

#endif
