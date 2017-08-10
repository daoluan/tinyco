#include "server.h"

#include <memory>
#include <string>
#include <fstream>
#include <streambuf>

#include "util/string.h"
#include "http/http_server.h"

namespace tinyco {

class SignalHelper {
 public:
  static void SetServerInstance(Server *srv) { srv_ = srv; }
  static void AllInOneCallback(int sig, siginfo_t *sig_info, void *unused) {
    if (srv_) srv_->SignalCallback(sig);
  }

 private:
  static Server *srv_;
};

Server *SignalHelper::srv_;

ServerImpl::ServerImpl() {}

ServerImpl::~ServerImpl() { Frame::Fini(); }

int ServerImpl::Initialize() {
  int ret = 0;
  if (!Frame::Init()) {
    LOG_ERROR("fail to init frame");
    return -__LINE__;
  }

  if (!ParseConfig()) {
    return -__LINE__;
  }
  LOG_INFO("parse config ok");

  LOG_INFO("init signal action");
  InitSigAction();

  LOG_INFO("init listener");
  if ((ret = InitSrv()) < 0) {
    LOG_INFO("fail to InitSrv: ret=%d", ret);
    return -__LINE__;
  }

  return 0;
}

bool ServerImpl::ParseConfig() {
  Json::CharReaderBuilder b;
  std::shared_ptr<Json::CharReader> reader(b.newCharReader());
  JSONCPP_STRING errs;

  std::ifstream t("./conf/tinyco.conf");
  std::string config_data((std::istreambuf_iterator<char>(t)),
                          std::istreambuf_iterator<char>());

  LOG_INFO("config = %s", config_data.c_str());
  if (!reader->parse(config_data.c_str(),
                     config_data.c_str() + config_data.size(), &config_,
                     &errs)) {
    LOG_ERROR("fail to parse config, please check config");
    return false;
  }

  if (!config_.isMember("udp") && config_.isMember("tcp") &&
      !config_.isMember("http")) {
    LOG_ERROR("no server item: udp, tcp or http");
    return false;
  }

  return true;
}

int ServerImpl::InitSigAction() {
  struct sigaction sa;
  sa.sa_sigaction = SignalHelper::AllInOneCallback;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, NULL);
}

struct ListenItem {
  std::string proto;
  network::IP ip;
  uint16_t port;

  bool Parse(const std::string &proto, const std::string &listen_config) {
    std::vector<std::string> items = string::Split(listen_config, ':');
    if (items.size() != 2) {
      return false;
    }

    this->proto = proto;
    network::IP ip;
    if (!network::GetEthAddr(items[1].c_str(), &ip)) {
      return false;
    }

    this->ip = ip;

    port = std::atoi(items[1].c_str());
    return true;
  }
};

int ServerImpl::InitListener(const std::string &proto) {
  ListenItem li;
  if (config_.isMember(proto)) {
    for (Json::ArrayIndex i = 0; i < config_[proto].size(); i++) {
      LOG_DEBUG("listen %s", config_[proto][i]["listen"].asString().c_str());
      if (li.Parse(proto, config_[proto][i]["listen"].asString())) {
        Listener *l = NULL;
        if ("tcp" == proto)
          l = new TcpListener();
        else
          l = new UdpListener();

        if (l->Listen(li.ip, li.port) < 0) {
          return -__LINE__;
        }
        LOG("proto=%s", proto.c_str());

        if ("tcp" == proto)
          Frame::CreateThread(new TcpSrvWork(l, this, this));
        else
          Frame::CreateThread(new UdpSrvWork(l, this));
      } else {
        return -__LINE__;
      }
    }
  }

  return 0;
}

int ServerImpl::InitSrv() {
  int ret = 0;
  if ((ret = InitListener("tcp")) < 0) {
    return -__LINE__;
  }

  if ((ret = InitListener("udp")) < 0) {
    return -__LINE__;
  }

  return 0;
}

int ServerImpl::Run() {
  std::shared_ptr<Thread> me(Frame::InitHereAsNewThread());

  while (true) {
    LOG_DEBUG("in server main loop");
    Frame::Sleep(1000);
    ServerLoop();
  }

  return 0;
}

int ServerImpl::ServerLoop() {}

void ServerImpl::SignalCallback(int signo) {
  if (signo == SIGUSR1) {
  }
}

void ServerImpl::FreeAllListener() {
  for (auto i = listeners_.begin(); i != listeners_.end(); i++) {
    (*i)->Destroy();
  }
}
}
