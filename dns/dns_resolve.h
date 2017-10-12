#ifndef DNS_RESOLVE_H_
#define DNS_RESOLVE_H_

// abstract class for dns resolving.
// you can implement your Resolve function by inheriting DNSResolver
#include <inttypes.h>
#include <string>

#include "util/network.h"

namespace tinyco {

namespace dns {

class DNSResolver {
 public:
  DNSResolver() {}
  virtual ~DNSResolver() {}

  virtual bool Resolve(const std::string &domain, network::IP *ip) = 0;
};
}
}

#endif
