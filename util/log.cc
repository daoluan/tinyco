#include "log.h"

#include <vector>
#include <string>
#include <stdarg.h>
#include <unistd.h>
#include <sstream>

#include "util/time.h"

namespace tinyco {

#define COMMONLOG()                       \
  va_list args;                           \
  va_start(args, fmt);                    \
  CommonLog(uin, line, func, fmt, &args); \
  va_end(args);

inline std::ifstream::pos_type filesize(const char *filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

LocalLog *LocalLog::Instance() {
  static LocalLog ll;
  return &ll;
}

int LocalLog::Initialize(const void *arg) {
  if (!arg) return -1;

  const char *filepath = static_cast<const char *>(arg);
  current_file_ = filepath;
  current_file_ += ".log";
  filepath_ = filepath;

  file_.open(current_file_.c_str(), std::ofstream::out | std::ofstream::app);
  return 0;
}

#define COMMONLOG()                       \
  va_list args;                           \
  va_start(args, fmt);                    \
  CommonLog(uin, line, func, fmt, &args); \
  va_end(args);

void LocalLog::Debug(uint64_t uin, uint32_t line, const char *func,
                     const char *fmt, ...) {
  if (loglevel_ > LL_DEBUG) return;

  COMMONLOG();
}

void LocalLog::Info(uint64_t uin, uint32_t line, const char *func,
                    const char *fmt, ...) {
  if (loglevel_ > LL_INFO) return;

  COMMONLOG();
}

void LocalLog::Warming(uint64_t uin, uint32_t line, const char *func,
                       const char *fmt, ...) {
  if (loglevel_ > LL_WARMING) return;

  COMMONLOG();
}

void LocalLog::Error(uint64_t uin, uint32_t line, const char *func,
                     const char *fmt, ...) {
  if (loglevel_ > LL_ERROR) return;

  COMMONLOG();
}

void Log::CommonLog(uint64_t uin, uint32_t line, const char *func,
                    const char *fmt, va_list *args) {
  content_.clear();
  AppendLogItemHeader(uin, line, func);
  char tmp[kLogItemSize];
  vsnprintf(tmp, sizeof(tmp), fmt, *args);
  content_.append(tmp);
  WriteLog();
}

uint32_t Log::AppendLogItemHeader(uint64_t uin, uint32_t line,
                                  const char *func) {
  std::stringstream ss;
  uint64_t now = time::mstime();
  time_t nows = static_cast<time_t>(now / 1000llu);

  auto tminfo = localtime(&nows);

  char tmp[32];
  uint32_t sz = strftime(tmp, sizeof(tmp), "[%Y:%m:%d %H:%M:%S", tminfo);
  ss << tmp << "." << std::to_string(now % 1000llu) << "]";
  ss << "[" << func << "]";
  ss << "[" << line << "]";
  ss << "[" << uin << "] ";
  content_ = ss.str();
  return content_.size();
}

void LocalLog::WriteLog() {
  const uint32_t kMaxFilesize = 1024 * 2014 * 100;
  const uint32_t kMaxFilsNum = 100;

  // write to file
  file_ << content_.c_str() << std::endl;
  file_.flush();
  return;

  // check log file size and rename log file if needed
  uint32_t fs = filesize(current_file_.c_str());
  if (fs < kMaxFilesize) {
    return;
  }

  for (uint32_t i = kMaxFilsNum - 1; i >= 0; i++) {

    std::string fp;
    if (i > 0)
      fp = filepath_ + "_" + std::to_string(i) + ".log";
    else
      fp = filepath_ + ".log";

    if (access(fp.c_str(), F_OK) != -1) {
      if (i == kMaxFilesize - 1) {
        printf("remove file\n");
        remove(fp.c_str());
      } else {
        printf("rename file\n");
        const std::string &fp_new =
            filepath_ + "_" + std::to_string(i + 1) + ".log";
        rename(fp.c_str(), fp_new.c_str());
      }
    }  // if access
  }    // for
}
}
