#include "mutex.h"

#include <fcntl.h>
#include <unistd.h>

int FileMtx::InitMtx(void *arg) {
  std::string lf = static_cast<char *>(arg);
  fd_ =
      open(lf.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd_ != -1) {
    const char *byte_to_write = "*";
    for (auto byte_count = 0; byte_count < 10; byte_count++) {
      write(fd_, byte_to_write, 1);
    }

    // trick: no need to remove it
    unlink(lf.c_str());
    return 0;
  }

  return -1;
}

int FileMtx::TryLock() {
  struct flock fl = {0};
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;

  if (fcntl(fd_, F_SETLK, &fl) == -1) {
    return -1;
  }

  return 0;
}

int FileMtx::Unlock() {
  struct flock fl = {0};

  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_SET;

  if (fcntl(fd_, F_SETLK, &fl) == -1) {
    return -1;
  }

  return 0;
}

#if (__amd64__ || __amd64)

inline uint64_t AtomicCompareAndSet(uint64_t *a, uint64_t old, uint64_t n) {
  u_char res;

  __asm__ volatile(
      "    cmpxchgq  %3, %1;   "
      "    sete      %0;       "
      : "=a"(res)
      : "m"(*a), "a"(old), "r"(n)
      : "cc", "memory");
  return res;
}

#else

inline uint64_t AtomicCompareAndSet(uint64_t *a, uint64_t old, uint64_t n) {
  *a = n;
  return n;
}

#endif

AtomicMtx::~AtomicMtx() {
  if (ptr_) munmap(ptr_, sizeof(*ptr_));
}

int AtomicMtx::InitMtx(void *arg) {
  ptr_ = (uint64_t *)mmap(NULL, sizeof(uint64_t), PROT_READ | PROT_WRITE,
                          MAP_ANON | MAP_SHARED, -1, 0);

  *ptr_ = 0;
  return 0;
}

int AtomicMtx::TryLock() {
  if (!ptr_) return -1;
  return (*ptr_ == 0 && AtomicCompareAndSet(ptr_, 0, getpid())) ? 0 : -1;
}

int AtomicMtx::Unlock() {
  if (!ptr_) return -1;
  return AtomicCompareAndSet(ptr_, getpid(), 0) ? 0 : -1;
}
