#include "mutex.h"

#include <fcntl.h>
#include <unistd.h>

int FileMtx::OpenLockFile(const std::string &lf) {
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
