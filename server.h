#ifndef SERVER_H_
#define SERVER_H_

#include <sys/sysinfo.h>
#include <set>
#include <map>
#include <memory>
#include <google/protobuf/service.h>

#include "frame.h"
#include "util/network.h"

namespace Json {
class Value;
};

namespace tinyco {

class Server;
class TcpReqWork;
class UdpReqWork;

class BusinessWorkBuilder {
 public:
  virtual TcpReqWork *BuildStreamBusinessWork(uint32_t port) = 0;
  virtual UdpReqWork *BuildUdpBusinessWork(uint32_t port) = 0;
};

class ConnTracker {
 public:
  ConnTracker() : graceful_shutdown_(false) {}
  struct Conn {};

  void NewConn(int fd, const Conn &conn) {
    conns_.empty();
    conns_[fd] = conn;
  }
  void RemoveConn(int fd) { conns_.erase(fd); }
  uint32_t GetConnSize() { return conns_.size(); }
  bool GracefulShutdown() const { return graceful_shutdown_; }

 protected:
  bool graceful_shutdown_;

 private:
  std::map<int, Conn> conns_;
};

class TcpReqWork : public Work {
 public:
  TcpReqWork() : sockfd_(-1), ct_(NULL) {}
  void SetFd(int sockfd) { sockfd_ = sockfd; }
  void SetConnTracker(ConnTracker *ct) { ct_ = ct; }
  virtual ~TcpReqWork() {
    if (ct_) ct_->RemoveConn(sockfd_);
    if (sockfd_ >= 0) close(sockfd_);
  }

 protected:
  int sockfd_;
  ConnTracker *ct_;
};

class TcpSrvWork : public Work {
 public:
  TcpSrvWork(Listener *listener, BusinessWorkBuilder *work_builder,
             ConnTracker *ct)
      : listener_(listener), work_builder_(work_builder), ct_(ct) {}
  virtual ~TcpSrvWork() { delete listener_; }

  int Run() {
    int ret = RunTcpSrv();
    if (ret < 0) {
      LOG("server error: %d", ret);
      exit(EXIT_FAILURE);
    }
  }

 private:
  int RunTcpSrv() {
    int accepted = 0;

    while (true) {
      // avoid thundering herd
      if (accepted++ > 15) {
        Frame::Sleep(50);
        accepted = 0;
        continue;
      }

      if (listener_->GetMtx()->TryLock() < 0) {
        // lock error, schedule out
        Frame::Sleep(50);
        continue;
      }

      LOG_DEBUG("accept on listenfd=%d", listener_->GetSocketFd());

      struct sockaddr_in server, client;
      socklen_t socklen = sizeof(client);
      auto fd = Frame::accept(listener_->GetSocketFd(),
                              (struct sockaddr *)&client, &socklen);
      if (fd < 0) {
        LOG("Frame::accept error: ret=%d|errno=%d", fd, errno);
        break;
      }

      listener_->GetMtx()->Unlock();

      if (network::SetNonBlock(fd) < 0) {
        LOG_ERROR("set nonblock error");
        break;
      }

      if (!work_builder_) {
        close(fd);
        continue;
      }

      auto b = work_builder_->BuildStreamBusinessWork(listener_->GetPort());
      b->SetFd(fd);

      if (ct_) {
        b->SetConnTracker(ct_);
        ConnTracker::Conn conn;
        ct_->NewConn(fd, conn);
      }

      Frame::CreateThread(b);

      if (ct_->GracefulShutdown()) {
        break;
      }
    }

    return 0;
  }

 private:
  Listener *listener_;
  BusinessWorkBuilder *work_builder_;
  ConnTracker *ct_;
};

struct UdpReqInfo {
  uint32_t sockfd;
  std::string reqpkg;
  sockaddr_in srcaddr;
  sockaddr_in dstaddr;
};

class UdpReqWork : public Work {
 public:
  UdpReqWork() {}
  virtual ~UdpReqWork() {}
  void SetReqInfo(const UdpReqInfo &req) { req_ = req; }
  int Run() = 0;
  int Reply(const std::string &rsp) {
    return Frame::sendto(req_.sockfd, rsp.data(), rsp.size(), 0,
                         (sockaddr *)&req_.srcaddr, sizeof(req_.srcaddr));
  }

 protected:
  UdpReqInfo req_;
};

class UdpSrvWork : public Work {
 public:
  UdpSrvWork(Listener *listener, BusinessWorkBuilder *work_builder)
      : listener_(listener), work_builder_(work_builder) {}
  virtual ~UdpSrvWork() { delete listener_; }
  int Run() {
    int ret = RunUdpSrv();
    if (ret < 0) {
      LOG("server error: %d", ret);
      exit(EXIT_FAILURE);
    }
  }

 private:
  int RunUdpSrv() {
    char recvbuf[65536];

    sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = listener_->GetIP().af_inet_ip;
    server.sin_port = htons(listener_->GetPort());
    while (true) {
      if (listener_->GetMtx()->TryLock() < 0) {
        // lock error, schedule out
        Frame::Sleep(500);
        continue;
      }

      sockaddr_in client = {0};
      socklen_t clisocklen = sizeof(client);
      int ret =
          Frame::recvfrom(listener_->GetSocketFd(), recvbuf, sizeof(recvbuf), 0,
                          (struct sockaddr *)&client, &clisocklen);
      if (ret > 0) {
        listener_->GetMtx()->Unlock();

        // create tread to handle request
        UdpReqInfo req;
        req.srcaddr = client;
        req.dstaddr = server;
        req.reqpkg.assign(recvbuf, ret);
        req.sockfd = listener_->GetSocketFd();

        if (!work_builder_) {
          continue;
        }

        auto b = work_builder_->BuildUdpBusinessWork(listener_->GetPort());
        b->SetReqInfo(req);

        Frame::CreateThread(b);
      } else if (ret < 0) {
        LOG("Frame::recvfrom error: %d", ret);
        return -__LINE__;
      }
    }

    return 0;
  }

  BusinessWorkBuilder *work_builder_;
  Listener *listener_;
};

inline int TcpSrv(uint32_t ip, uint16_t port,
                  BusinessWorkBuilder *work_builder) {
  auto l = new TcpListener();
  if (l->Listen(network::IP{ip}, port) < 0) return -__LINE__;

  Frame::CreateThread(new TcpSrvWork(l, work_builder, NULL));
  Frame::Schedule();
  delete l;
}

inline int UdpSrv(uint32_t ip, uint16_t port,
                  BusinessWorkBuilder *work_builder) {
  auto l = new UdpListener();
  if (l->Listen(network::IP{ip}, port) < 0) return -__LINE__;
  Frame::CreateThread(new UdpSrvWork(l, work_builder));
  Frame::Schedule();
  delete l;
}

class Server {
 public:
  Server() : sig_read_fd_(-1), sig_write_fd_(-1) {}
  virtual ~Server() {}
  virtual int Initialize(int argc, char *argv[]) = 0;
  virtual int Run() = 0;
  virtual void SignalCallback(int signo) = 0;
  virtual int GetSigReadFd() const { return sig_read_fd_; }
  virtual int GetSigWriteFd() const { return sig_write_fd_; }
  virtual int Daemonize() = 0;

 protected:
  virtual int ServerLoop() = 0;

  // use to notify eventloop to do signal action
  int sig_read_fd_;
  int sig_write_fd_;
};

class ServerImpl : public Server,
                   public BusinessWorkBuilder,
                   public ConnTracker {
 public:
  ServerImpl();
  virtual ~ServerImpl();
  virtual int Initialize(int argc, char *argv[]);
  virtual int Run();
  virtual void SignalCallback(int signo);

  // restful_mappings example:
  // "/v1/account/add > add,"
  // "/v1/account/del > del,"
  // "/v1/account/query > query"
  bool AddRpcService(google::protobuf::Service *service,
                     const std::string &restful_mappings);

  struct MethodDescriptor {
    google::protobuf::Service *service;
    std::string method;
    std::string full_name;
  };

 private:
  virtual int Daemonize();

  bool ParseConfig();
  virtual int ServerLoop();
  int InitSigAction();
  int InitSrv();
  int InitListener(const std::string &proto);
  void FreeAllListener();
  void SetProcTitle(const char *title);

  void MasterRun();
  void WorkerRun();

  std::shared_ptr<Json::Value> config_;
  std::set<Listener *> listeners_;
  std::set<int> clients_;

  // master
  struct Worker {
    std::string name;
    int pid;
  };

  std::vector<Worker> worker_processes_;

  enum WorkMode {
    WM_UNKNOWN = 0,
    WM_MASTER = 1,
    WM_WORKER = 2,
  };
  WorkMode mode_;

  // worker abnormal exit(segment fault, etc...)
  int restart_worker_pid_;
  void GetWorkerStatus();

 protected:
  std::unordered_map<std::string, MethodDescriptor> method_map_;
};

extern "C" {
void setproctitle(const char *fmt, ...);
void spt_init(int argc, char *argv[]);
}
}

#endif
