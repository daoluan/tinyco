#ifndef DNS_RESOLVE_IMPL_H_
#define DNS_RESOLVE_IMPL_H_

#include "dns_resolve.h"

#include <unordered_map>
#include <string>
#include <vector>

#include "util/network.h"

namespace tinyco {
namespace dns {
// not thread-safe
class DNSResolverImpl : public DNSResolver {
 public:
  DNSResolverImpl() {}
  virtual ~DNSResolverImpl() {}
  virtual network::IP Resolve(const std::string &domain);

 private:
  struct ResCache {
    std::vector<network::IP> ip;
    uint32_t timeout;  // timeout unix timestmap
    uint32_t idx;      // return next time

    network::IP PickOne() {
      network::IP res;
      res.af_inet_ip = 0;
      if (ip.empty()) return res;

      return ip[(idx++) % ip.size()];
    }
  };

 private:
  static std::unordered_map<std::string, ResCache> res_cache_;
};
}
}

#endif
