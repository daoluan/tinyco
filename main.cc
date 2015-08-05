#include <string>
#include <iostream>
#include <inttypes.h>
#include <ucontext.h>
#include <stdio.h>
#include <list>
#include <stdlib.h>
#include <string.h>
#include "frame.h"
#include "thread.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <string>
#include <fcntl.h>

class TestWork : public Work {
public:
  TestWork () {}
  virtual ~TestWork () {}

  int Run() {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
    addr.sin_port = htons(32000);

    int fd = socket(AF_INET,SOCK_DGRAM,0);
    int flags = fcntl(fd,F_GETFL,0);
    if (flags < 0) return -1;
    flags = flags|O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    std::string sendbuf = "hello";

    printf("send: %s\n", sendbuf.c_str());

    std::string recvbuf;
    int ret = Frame::UdpSendAndRecv(fd,sendbuf,addr,&recvbuf);
    if (ret < 0)
    {
      fprintf(stderr, "UdpSendAndRecv error: ret=%d\n", ret);
      return -1;
    }

    printf("recv: %s\n", recvbuf.c_str());
  }
private:
};

// struct Thread
// {
//   ucontext_t uc;
//   bool uc_init;
//   char *stack;
//   int state;
//   int block_cnt;
//   const static int kMaxBlockCnt = 5;
//
//   typedef void (*Cb)(Thread *);
//   Cb cb;
//
//   Thread() : state(0), cb(NULL), block_cnt(0)
//   {
//     stack = new char[1024];
//   }
// };
//
// std::list<Thread> runnable_list;
// Thread main_thread;
//
// void schedule(std::list<Thread> &runnable_list, Thread &running);
//
// void context_test_func(Thread *running)
// {
//   printf("%s\n",__FUNCTION__);
//   printf("i am blocking\n");
//   running->state = 1;
//   schedule(runnable_list,*running);
//   printf("thread over\n");
//   running->state = 2;
// }
//
// void schedule(std::list<Thread> &runnable_list, Thread &running)
// {
//   std::list<Thread>::iterator it;
//   it = runnable_list.begin();
//   for (; it!=runnable_list.end(); it++)
//   {
//     if (it->block_cnt < Thread::kMaxBlockCnt && it->state == 0)
//     {
//       // runnable_list.push_back(*it);
//       printf("hello coroutine\n");
//       swapcontext(&running.uc,&it->uc);
//     }
//   }
//
//   if (it == runnable_list.end())
//   {
//     it = runnable_list.begin();
//     for (; it!=runnable_list.end(); it++)
//     {
//       if (it->state == 1)
//       swapcontext(&running.uc,&it->uc);
//     }
//   }
// }
#include <unordered_map>
int main()
{
  int fd = socket(AF_INET,SOCK_DGRAM,0);
  if (fd < 0)
  {
    fprintf(stderr, "socket error\n");
    return -1;
  }

  assert(Frame::Init());
  std::shared_ptr<Thread> thread(new Thread());
  assert(thread->Init());
  TestWork work;
  thread->SetContext(Thread::BeginThread,work,
    *Frame::main_thread_);
  Frame::thread_runnable_.push_back(thread);
  Frame::Process();

  // printf("begin\n");

  // Thread thread0, thread1, thread2;
  // runnable_list.push_back(thread0);
  // runnable_list.push_back(thread1);
  // runnable_list.push_back(thread2);

  // printf("init main_thread\n");
  // getcontext(&main_thread.uc);
  // main_thread.uc.uc_link = 0;
  // main_thread.uc.uc_stack.ss_sp = main_thread.stack;
  // main_thread.uc.uc_stack.ss_size = 1024;
  // makecontext(&main_thread.uc,(void (*)(void))context_test_func,1,&main_thread);

  // printf("xxxxxxxxxxxxxxxxx\n");

  // std::list<Thread>::iterator it;
  // for (it=runnable_list.begin(); it!=runnable_list.end(); it++)
  // {
  //   getcontext(&it->uc);
  //   it->uc.uc_link = &main_thread.uc;
  //   it->uc.uc_stack.ss_sp = it->stack;
  //   it->uc.uc_stack.ss_size = 1024;
  //   makecontext(&it->uc,(void (*)(void))context_test_func,1,&(*it));
  // }

  // while (true)
  // {
  //   schedule(runnable_list,main_thread);
  //   printf("end while\n");
  // }

  // printf("end\n");
  return 0;
}
