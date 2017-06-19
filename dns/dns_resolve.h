#ifndef DNS_RESOLVE_H_
#define DNS_RESOLVE_H_

// abstract class for dns resolving.
// you can implement your Resolve function by inheriting DNSResolver
#include <inttypes.h>
#include <string>

namespace tinyco {
namespace dns {

class DNSResolver {
 public:
  DNSResolver() {}
  virtual ~DNSResolver() {}

  union IP {
    uint32_t af_inet_ip;
    uint64_t af_inet6_ip;
  };

  virtual IP Resolve(const std::string &domain) = 0;
};
}
}

#endif
