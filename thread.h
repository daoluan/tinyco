#ifndef THREAD_H__
#define THREAD_H__

#include <ucontext.h>
#include "work.h"
#include <cstddef>
#include <cstdio>
#include <cstring>

class Stack {
public:
  Stack (size_t size)
  {
    if (size > kMaxStackSize)
      size = kMaxStackSize;
    size_ = size;
    stack_ = new char[size_];
  }

  Stack () : stack_(NULL), size_(0) {}

  virtual ~Stack () {
    delete stack_;
    stack_ = NULL;
  }

  bool Init (size_t size) {
    if (stack_) delete stack_;

    size_ = size;
    stack_ = new char[size_];
    return true;
  }

  char *GetStackBeginPoint() {
    return stack_;
  }

  size_t Size() {
    return size_;
  }

public:
  const static size_t kMaxStackSize = 1024;

private:
  char *stack_;
  size_t size_;
};

class Thread {
public:
  Thread ();
  virtual ~Thread ();

  bool Init() {
    if (!stack_.Init(Stack::kMaxStackSize))
    {
      fprintf(stderr, "stack init error\n");
      return false;
    }

    state_ = THREAD_STATE_STOP;
    ::memset(&uc_,sizeof(uc_),0);
    getcontext(&uc_);
    return true;
  }

  enum {
    THREAD_STATE_STOP,
    THREAD_STATE_RUNNABLE,
  };

  void SetState(int state) {
    state_ = state;
  }

  // 主动让出 CPU
  void ScheduleYield();

  int SwapContext(Thread *thread) {
    int ret = swapcontext(&uc_,thread->MutableContext());
    return 0;
  }

  // 线程真正开始执行地方
  // why could not be const Work &work
  static int BeginThread(Work &work) {
    int ret = work.Run();
    return 0;
  }

  typedef int (* BeginFrom)(Work &work);
  typedef void (* ContextFun)();
  void SetContext() {
    uc_.uc_stack.ss_sp = stack_.GetStackBeginPoint();
    uc_.uc_stack.ss_size = stack_.Size();
    uc_.uc_link = 0;
    makecontext(&uc_,reinterpret_cast<ContextFun>(Thread::BeginThread),0);
  }

  void SetContext(BeginFrom beginfrom, const Work &work, const Thread &parent) {
    parent_ = const_cast<Thread *>(&parent);
    fun_ = beginfrom;
    uc_.uc_stack.ss_sp = stack_.GetStackBeginPoint();
    uc_.uc_stack.ss_size = stack_.Size();
    uc_.uc_link = const_cast<Thread *>(&parent)->MutableContext();
    makecontext(&uc_,reinterpret_cast<ContextFun>(fun_),1,&work);
  }

  int GetThreadId() {
    return thread_id_;
  }

  int SetThreadId(int threadid) {
    thread_id_ = threadid;
  }

  ucontext_t *MutableContext() {
    return &uc_;
  }

private:
  BeginFrom fun_;
  Stack stack_;
  int state_;
  ucontext_t uc_;
  int thread_id_;
  Thread *parent_;
};

#endif
