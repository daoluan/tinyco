#ifndef MUTEX_H_
#define MUTEX_H_

#include <string>

class Mutex {
 public:
  virtual int TryLock() = 0;
  virtual int Unlock() = 0;
};

class FileMtx : public Mutex {
 public:
  int OpenLockFile(const std::string &lf);
  virtual int TryLock();
  virtual int Unlock();

 private:
  int fd_;
};

#endif
