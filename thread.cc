#include "thread.h"

#include "frame.h"

namespace tinyco {
bool Stack::Init(size_t size) {
  if (stack_) delete stack_;

  size_ = size;
  stack_ = new char[size_];
  return true;
}

char *Stack::GetStackBeginPoint() { return stack_; }

size_t Stack::Size() { return size_; }

Thread::Thread() {}

Thread::~Thread() {}

bool Thread::Init() {
  if (!stack_.Init(Stack::kMaxStackSize)) {
    return false;
  }

  state_ = TS_STOP;
  return true;
}

void Thread::RestoreContext() { setcontext(&uc_); }

void Thread::Schedule() { getcontext(&uc_); }

void Thread::SetContext(BeginFrom func, void *arg) {
  fun_ = func;
  getcontext(&uc_);
  uc_.uc_stack.ss_sp = stack_.GetStackBeginPoint();
  uc_.uc_stack.ss_size = stack_.Size();
  uc_.uc_link = NULL;
  uc_.uc_stack.ss_flags = 0;
  makecontext(&uc_, reinterpret_cast<ContextFun>(func), 1, arg);
}
}
