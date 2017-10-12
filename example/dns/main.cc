#include "frame.h"

#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <netinet/in.h>

#include "dns/dns_resolve_impl.h"

using namespace tinyco;

class TestWork : public Work {
 public:
  TestWork() {}
  virtual ~TestWork() {}

  int Run() {
    const std::vector<std::string> domains = {"www.baidu.com",  "www.qq.com",
                                              "www.taobao.com", "daoluan.net"};
    dns::DNSResolverImpl dri;
    network::IP ip;
    for (auto i = 0; i < domains.size(); i++) {
      auto bret = dri.Resolve(domains[i], &ip);
      if (bret)
        LOG("%s->%s", domains[i].c_str(), inet_ntoa(in_addr{ip.af_inet_ip}));
      else
        LOG("%s reslove error", domains[i].c_str());
    }

    // use cache
    LOG("## use dns cache ##");
    for (auto i = 0; i < domains.size(); i++) {
      auto bret = dri.Resolve(domains[i], &ip);
      if (bret)
        LOG("%s->%s", domains[i].c_str(), inet_ntoa(in_addr{ip.af_inet_ip}));
      else
        LOG("%s reslove error", domains[i].c_str());
    }
    return 0;
  }
};

int main() {
  assert(Frame::Init());
  Frame::CreateThread(new TestWork);
  Frame::Schedule();
  Frame::Fini();

  return 0;
}
