#include "dns_resolve_impl.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ares
#include "ares.h"
#include "nameser.h"
// run "cmake ." in c-ares to generate ares_config.h
#include "ares_config.h"
#include "ares_private.h"

#include "frame.h"

namespace tinyco {
namespace dns {
std::unordered_map<std::string, DNSResolverImpl::ResCache>
    DNSResolverImpl::res_cache_;

// not thread-safe
static ares_channel channel;
static bool ares_channel_init = false;

network::IP DNSResolverImpl::Resolve(const std::string &domain) {
  network::IP res;
  res.af_inet_ip = 0;
  int ret = 0;

  if (!ares_channel_init) {
    struct ares_options options;
    int optmask = 0;

    ret = ares_library_init(ARES_LIB_INIT_ALL);
    if (ret != ARES_SUCCESS) {
      return res;
    }

    ret = ares_init_options(&channel, &options, optmask);
    if (ret != ARES_SUCCESS) {
      return res;
    }

    ares_channel_init = true;
  }

  // check cache
  auto c = res_cache_.find(domain);
  if (c != res_cache_.end() && c->second.timeout > time(NULL)) {
    return c->second.PickOne();
  }

  // resolve
  unsigned char *buf;
  std::string rsp;
  int buflen = 0;
  ret = ares_mkquery(domain.c_str(), ns_c_in, T_A, 1234, 0, &buf, &buflen);
  if (ret != 0) {
    return res;
  }

  if (channel->nservers <= 0) return res;
  in_addr dnsip = (channel->servers[0].addr.addr.addr4);

  struct sockaddr_in addr;
  addr.sin_addr = dnsip;
  addr.sin_port = htons(53);
  addr.sin_family = AF_INET;

  std::string req;
  req.assign(reinterpret_cast<char *>(buf), buflen);
  ret = Frame::UdpSendAndRecv(req, addr, &rsp);
  if (ret < 0) {
    return res;
  }

  hostent *ht;
  const unsigned char *p = reinterpret_cast<const unsigned char *>(rsp.data());
  ret = ares_parse_a_reply(p, rsp.size(), &ht, NULL, NULL);
  if (!ht) {
    return res;
  }

  auto &n = res_cache_[domain];
  n.timeout = time(NULL) + 5 * 60;
  for (auto i = 0; ht->h_addr_list[i]; ++i) {
    res.af_inet_ip = *reinterpret_cast<uint32_t *>(ht->h_addr_list[i]);
    n.ip.push_back(res);
  }

  return n.PickOne();
}
}
}
