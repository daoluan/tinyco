#include "server.h"

#include <memory>
#include <string>
#include <fstream>
#include <streambuf>
#include <fcntl.h>

#include "util/string.h"
#include "http/http_server.h"

namespace tinyco {

class SignalHelper : public Work {
 public:
  static void SetServerInstance(Server *srv) { srv_ = srv; }
  static void AllInOneCallback(int sig, siginfo_t *sig_info, void *unused) {
    LOG_DEBUG("write signal notify fd=%d|sig=%d", srv_->GetSigWriteFd(), sig);
    int ret = Frame::send(srv_->GetSigWriteFd(), &sig, sizeof(sig), 0);
    LOG_DEBUG("ret = %d", ret);
  }

  int Run() {
    while (true) {
      int signo = 0;
      LOG_DEBUG("ready to recv signal notify fd=%d", srv_->GetSigReadFd());
      int ret = Frame::recv(srv_->GetSigReadFd(), &signo, sizeof(signo), 0);
      if (ret < 0) {
        LOG_ERROR("read notifyfd_ error: ret=%d|socket=%d|error=%d", ret,
                  srv_->GetSigReadFd(), errno);
        return 0;
      }

      if (srv_) srv_->SignalCallback(signo);
    }

    return 0;
  }

 private:
  static Server *srv_;
};

Server *SignalHelper::srv_;

ServerImpl::ServerImpl() {}

ServerImpl::~ServerImpl() { Frame::Fini(); }

int ServerImpl::Daemonize() {
  int fd;

  if (fork() != 0) exit(0);  // parent exits
  setsid();                  // create a new session

  if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO) close(fd);  // if not, hold them
  }

  return 0;
}

int ServerImpl::Initialize(int argc, char *argv[]) {
  int ret = 0;

  if (LocalLog::Instance()->Initialize("tinyco") < 0) {
    fprintf(stderr, "fail init log util\n");
    return -__LINE__;
  }

  if (!Frame::Init()) {
    fprintf(stderr, "fail to init frame");
    return -__LINE__;
  }

  if (!ParseConfig()) {
    fprintf(stderr, "fail to parse config");
    return -__LINE__;
  }

  if (InitSigAction() < 0) {
    fprintf(stderr, "fail to init sigaction");
    return -__LINE__;
  }

  if ((ret = InitSrv()) < 0) {
    fprintf(stderr, "fail to InitSrv: ret=%d", ret);
    return -__LINE__;
  }

  spt_init(argc, argv);
  SetProcTitle("tinyco: worker");

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
  SignalHelper::SetServerInstance(this);
  struct sigaction sa;
  sa.sa_sigaction = SignalHelper::AllInOneCallback;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, NULL);

  int sockpair[2] = {0};
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair) == -1) {
    printf("create unnamed socket pair failed:%s\n", strerror(errno));
    exit(-1);
  }

  // 0 read
  // 1 write
  sig_read_fd_ = sockpair[0];
  sig_write_fd_ = sockpair[1];
  network::SetNonBlock(sig_read_fd_);
  network::SetNonBlock(sig_write_fd_);
  LOG_INFO("signal notify socket: read=%d|write=%d", sig_read_fd_,
           sig_write_fd_);

  Frame::CreateThread(new SignalHelper());
  return 0;
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
    if (!network::GetEthAddr(items[0].c_str(), &ip)) {
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
        listeners_.insert(l);

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

  Daemonize();

  return 0;
}

int ServerImpl::Run() {
  std::shared_ptr<Thread> me(Frame::InitHereAsNewThread());

  while (true) {
    LOG_DEBUG("in server main loop");
    Frame::Sleep(1000);
    ServerLoop();

    if (GracefulShutdown()) {
      if (GetConnSize() == 0) {
        LOG_DEBUG("graceful shutdown");
        break;
      }
    }
  }

  return 0;
}

int ServerImpl::ServerLoop() { return 0; }

void ServerImpl::SignalCallback(int signo) {
  LOG_DEBUG("recv signo = %d", signo);
  switch (signo) {
    case SIGUSR1:
      break;
    case SIGHUP:
      graceful_shutdown_ = true;
      break;
  }
}

void ServerImpl::FreeAllListener() {
  for (auto i = listeners_.begin(); i != listeners_.end(); i++) {
    (*i)->Destroy();
  }
}

void ServerImpl::SetProcTitle(const char *title) { setproctitle("%s", title); }
}
