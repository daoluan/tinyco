#ifndef TINYCO_TIME_H_
#define TINYCO_TIME_H_

#include <time.h>

namespace tinyco {
namespace time {

inline uint64_t mstime() {
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (unsigned long long)(tv.tv_sec) * 1000 +
         (unsigned long long)(tv.tv_usec) / 1000;
}
}
}

#endif
