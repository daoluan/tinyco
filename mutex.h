#ifndef MUTEX_H_
#define MUTEX_H_

#include <string>
#include <sys/mman.h>
#include <unistd.h>

class Mutex {
 public:
  virtual int InitMtx(void *arg) = 0;
  virtual int TryLock() = 0;
  virtual int Unlock() = 0;
};

class DummyMtx : public Mutex {
 public:
  int InitMtx(void *arg) { return 0; }
  virtual int TryLock() { return 0; }
  virtual int Unlock() { return 0; }
};

class FileMtx : public Mutex {
 public:
  int InitMtx(void *arg);
  virtual int TryLock();
  virtual int Unlock();

 private:
  int fd_;
};

class AtomicMtx : public Mutex {
 public:
  int InitMtx(void *arg);
  virtual ~AtomicMtx();

  virtual int TryLock();
  virtual int Unlock();

 private:
  uint64_t *ptr_;
};

#endif
