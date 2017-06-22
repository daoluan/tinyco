#ifndef THREAD_H_
#define THREAD_H_

#include <ucontext.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <event.h>

namespace tinyco {
class Stack {
 public:
  Stack(size_t size) {
    if (size > kMaxStackSize) size = kMaxStackSize;
    size_ = size;
    stack_ = new char[size_];
  }

  Stack() : stack_(NULL), size_(0) {}

  virtual ~Stack() {
    delete stack_;
    stack_ = NULL;
  }

  bool Init(size_t size);

  char *GetStackBeginPoint();

  size_t Size();

 public:
  const static size_t kMaxStackSize = 128 * 1024;

 private:
  char *stack_;
  size_t size_;
};

class Thread {
 public:
  Thread();
  virtual ~Thread();

  bool Init();

  enum ThreadState {
    TS_STOP,
    TS_RUNNABLE,
    TS_PENDING,
  };

  void SetState(int state) { state_ = state; }

  // restore me
  void RestoreContext();

  // schedule me out
  void Schedule();

  typedef int (*BeginFrom)(void *argc);
  typedef void (*ContextFun)();
  void SetContext(BeginFrom func, void *argc);

  void Pending(uint64_t ms) { wakeup_ts_ = ms; }

  uint64_t GetWakeupTime() const { return wakeup_ts_; }

 private:
  BeginFrom fun_;
  Stack stack_;
  int state_;
  ucontext_t uc_;
  int thread_id_;
  Thread *parent_;
  uint64_t wakeup_ts_;
};
}

#endif
