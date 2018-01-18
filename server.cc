#include "server.h"

#include <memory>
#include <string>
#include <fstream>
#include <streambuf>
#include <fcntl.h>
#include <sys/wait.h>
#include <google/protobuf/descriptor.h>

#include "util/string.h"
#include "http/http_server.h"
#include "json/json.h"

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

ServerImpl::ServerImpl() : mode_(WM_UNKNOWN) {}

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
  config_.reset(new Json::Value);
  int ret = 0;

  // set proc signal mask
  sigset_t newset, oldset;
  sigemptyset(&newset);
  sigaddset(&newset, SIGCHLD);
  sigaddset(&newset, SIGHUP);
  sigaddset(&newset, SIGUSR1);
  sigprocmask(SIG_BLOCK, &newset, NULL);

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

  int worker_num = get_nprocs() > 0 ? get_nprocs() : 1;
  int childpid = 0;
  const std::string &worker_title = "tinyco: worker";
  for (auto i = 0; i < worker_num; i++) {
    if ((childpid = fork()) == 0) break;
    if (childpid < 0) {
      fprintf(stderr, "fork error");
      return -__LINE__;
    }

    Worker w = {worker_title, childpid};
    worker_processes_.push_back(w);
  }

  // child init
  if (0 == childpid) {

    mode_ = WM_WORKER;
    sigemptyset(&newset);
    sigprocmask(SIG_SETMASK, &newset, NULL);

    SetProcTitle(worker_title.c_str());

    return 0;
  }

  // master init
  mode_ = WM_MASTER;
  sigemptyset(&newset);
  sigprocmask(SIG_SETMASK, &newset, NULL);

  SetProcTitle("tinyco: master");

  return 0;
}

bool ServerImpl::AddRpcService(google::protobuf::Service *service,
                               const std::string &restful_mappings) {
  // contruct service map
  auto methods = string::Split(restful_mappings, ',');
  for (auto &method : methods) {
    auto path_to_method = string::Split(method, '>');

    if (path_to_method.size() != 2) return false;

    string::trim(path_to_method[0]);
    string::trim(path_to_method[1]);

    MethodDescriptor md;
    md.service = service;
    md.method = path_to_method[1];

    // check whether method is in service
    auto pmd = service->GetDescriptor()->FindMethodByName(md.method);
    if (!pmd) {
      LOG_ERROR("no corresponding method: %s", md.method.c_str());
      return false;
    }

    md.full_name = pmd->full_name();

    // path to method descriptor
    method_map_[path_to_method[0]] = md;
  }

  for (auto &ite : method_map_) {
    LOG_INFO("%s => %s.%s added", ite.first.c_str(),
             ite.second.service->GetDescriptor()->full_name().c_str(),
             ite.second.method.c_str());
  }

  return true;
}

bool ServerImpl::ParseConfig() {
  Json::CharReaderBuilder b;
  std::shared_ptr<Json::CharReader> reader(b.newCharReader());
  JSONCPP_STRING errs;

  std::ifstream t("./conf/tinyco.json");
  std::string config_data((std::istreambuf_iterator<char>(t)),
                          std::istreambuf_iterator<char>());

  LOG_INFO("config = %s", config_data.c_str());
  if (!reader->parse(config_data.c_str(),
                     config_data.c_str() + config_data.size(), config_.get(),
                     &errs)) {
    LOG_ERROR("fail to parse config, please check config");
    return false;
  }

  if (!config_->isMember("udp") && config_->isMember("tcp") &&
      !config_->isMember("http")) {
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
  sigaction(SIGCHLD, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);

  int sockpair[2] = {0};
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair) == -1) {
    fprintf(stderr, "create unnamed socket pair failed:%s\n", strerror(errno));
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
  const Json::Value &config = (*config_);
  if (config_->isMember(proto)) {
    for (Json::ArrayIndex i = 0; i < config[proto].size(); i++) {
      LOG_DEBUG("listen %s", config[proto][i]["listen"].asString().c_str());
      if (li.Parse(proto, config[proto][i]["listen"].asString())) {
        Listener *l = NULL;
        if ("tcp" == proto)
          l = new TcpListener();
        else if ("udp" == proto)
          l = new UdpListener();
        else
          continue;

        if (l->Listen(li.ip, li.port) < 0) {
          return -__LINE__;
        }
        LOG_INFO("proto=%s", proto.c_str());
        listeners_.insert(l);
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

  if (WM_MASTER == mode_) {
    MasterRun();
  } else if (WM_WORKER == mode_) {
    WorkerRun();
  } else {
    fprintf(stderr, "unknown work mode");
    return -__LINE__;
  }

  return 0;
}

int ServerImpl::ServerLoop() { return 0; }

void ServerImpl::SignalCallback(int signo) {
  LOG_DEBUG("recv signo = %d", signo);
  if (WM_MASTER == mode_) {
    switch (signo) {
      case SIGCHLD:
        GetWorkerStatus();
        break;
    }
  } else if (WM_WORKER == mode_) {
    switch (signo) {
      case SIGHUP:
        graceful_shutdown_ = true;
        break;
    }
  }
}

void ServerImpl::FreeAllListener() {
  for (auto i = listeners_.begin(); i != listeners_.end(); i++) {
    (*i)->Destroy();
  }
}

void ServerImpl::SetProcTitle(const char *title) { setproctitle("%s", title); }

void ServerImpl::MasterRun() {
  LOG_DEBUG("master run");

  while (true) {
    Frame::Sleep(1000);
    LOG_DEBUG("in master main loop");

    for (auto &pid : restart_worker_pid_) {
      LOG_INFO("pid=%d", pid);
      Worker *w = NULL;
      for (auto &ite : worker_processes_) {
        if (ite.pid == pid) {
          w = &ite;
        }
      }

      uint64_t check_data = pid;
      for (auto &ite : listeners_) {
        ite->GetMtx()->ForcedUnlockIfNeed(&check_data);
      }

      int new_worker_pid = fork();
      if (0 == new_worker_pid) {
        SetProcTitle("tinyco: worker");
        WorkerRun();
        break;
      } else if (new_worker_pid > 0) {
        w->pid = new_worker_pid;
      } else {
        LOG_ERROR("WARNING: fail to restart");
        continue;
      }
    }

    restart_worker_pid_.clear();
  }
}

void ServerImpl::WorkerRun() {
  LOG_DEBUG("worker run");

  for (auto ite : listeners_) {
    if (ite->GetProto() == "tcp") {
      Frame::CreateThread(new TcpSrvWork(ite, this, this));
    } else if (ite->GetProto() == "udp") {
      Frame::CreateThread(new UdpSrvWork(ite, this));
    } else
      continue;
  }

  while (true) {
    LOG_DEBUG("in worker main loop");
    Frame::Sleep(1000);
    ServerLoop();

    if (GracefulShutdown()) {
      if (GetConnSize() == 0) {
        LOG_DEBUG("graceful shutdown");
        break;
      }
    }
  }
}
void ServerImpl::GetWorkerStatus() {
  int pid;
  int status;
  for (;;) {
    pid = waitpid(-1, &status, WNOHANG);

    if (0 == pid) {
      return;
    }

    if (-1 == pid) {
      if (EINTR == errno) {
        continue;
      }

      return;
    }

    int restart_pid = 0;
    for (auto &w : worker_processes_) {
      if (w.pid == pid) {
        restart_pid = pid;
        LOG_INFO("worker %d exit and restart", pid);
        break;
      }
    }

    if (-1 == restart_pid) {
      LOG_ERROR("WARNING: unknow pid: %d", pid);
    }

    restart_worker_pid_.push_back(restart_pid);
    return;
  }
}
}
