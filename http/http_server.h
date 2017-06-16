#include "frame.h"
#include "http_request.h"
#include "http_response.h"

#include <unistd.h>

namespace tinyco {
namespace http {

class HttpSrvWork : public Work {
 public:
  void SetFd(int sockfd) { sockfd_ = sockfd; }
  virtual ~HttpSrvWork() { close(sockfd_); }
  virtual int Run();
  // implement you function
  virtual int Serve() = 0;
  virtual int Reply();

 protected:
  HttpRequest hreq_;
  HttpResponse hrsp_;

 private:
  int sockfd_;
};
}
}
