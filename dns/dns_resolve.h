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

  virtual network::IP Resolve(const std::string &domain) = 0;
};
}
}

#endif
