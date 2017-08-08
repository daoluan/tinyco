#include "server.h"
#include "http_request.h"
#include "http_response.h"

#include <unistd.h>

namespace tinyco {
namespace http {

class HttpSrvWork : public TcpReqWork {
 public:
  virtual int Run();
  // implement you function
  virtual int Serve() = 0;
  virtual int Reply();

 protected:
  HttpRequest hreq_;
  HttpResponse hrsp_;
};
}
}
