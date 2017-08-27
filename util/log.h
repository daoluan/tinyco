
#ifndef LOG_H_
#define LOG_H_

#include <fstream>

namespace tinyco {

enum LOGLEVEL {
  LL_DEBUG = 0,
  LL_INFO,
  LL_WARMING,
  LL_ERROR,
};

class Log {
 public:
  virtual int Initialize(const void *arg) = 0;
  virtual void SetLogLevel(int level) { loglevel_ = level; }
  virtual void Debug(uint64_t uin, uint32_t line, const char *func,
                     const char *fmt, ...) = 0;
  virtual void Info(uint64_t uin, uint32_t line, const char *func,
                    const char *fmt, ...) = 0;
  virtual void Warming(uint64_t uin, uint32_t line, const char *func,
                       const char *fmt, ...) = 0;
  virtual void Error(uint64_t uin, uint32_t line, const char *func,
                     const char *fmt, ...) = 0;

 protected:
  virtual void CommonLog(uint64_t uin, uint32_t line, const char *func,
                         const char *fmt, va_list *args);

  virtual uint32_t AppendLogItemHeader(uint64_t uin, uint32_t line,
                                       const char *func);

  virtual void WriteLog() {}

 protected:
  int loglevel_;
  std::string content_;
  const static uint32_t kLogItemSize = 2048;
};

class LocalLog : public Log {
 public:
  static LocalLog *Instance();

  virtual int Initialize(const void *arg);

  virtual void Debug(uint64_t uin, uint32_t line, const char *func,
                     const char *fmt, ...);

  virtual void Info(uint64_t uin, uint32_t line, const char *func,
                    const char *fmt, ...);

  virtual void Warming(uint64_t uin, uint32_t line, const char *func,
                       const char *fmt, ...);

  virtual void Error(uint64_t uin, uint32_t line, const char *func,
                     const char *fmt, ...);

 private:
  virtual void WriteLog();

 private:
  std::string filepath_;
  std::string current_file_;
  std::ofstream file_;
};

#if 0
#define LOG(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)

#define LOG_ERROR(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)

#define LOG_WARMING(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)

#define LOG_NOTICE(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)

#define LOG_INFO(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)

#define LOG_DEBUG(fmt, arg...) \
  printf("[%s][%s][%u]: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg)
#endif

#if 1
#define LOG(fmt, arg...) \
  LocalLog::Instance()->Debug(LL_DEBUG, __LINE__, __FUNCTION__, fmt, ##arg)

#define LOG_ERROR(fmt, arg...) \
  LocalLog::Instance()->Error(LL_ERROR, __LINE__, __FUNCTION__, fmt, ##arg)

#define LOG_WARMING(fmt, arg...) \
  LocalLog::Instance()->Warming(LL_WARMING, __LINE__, __FUNCTION__, fmt, ##arg)

#define LOG_INFO(fmt, arg...) \
  LocalLog::Instance()->Info(LL_INFO, __LINE__, __FUNCTION__, fmt, ##arg)

#define LOG_DEBUG(fmt, arg...) \
  LocalLog::Instance()->Debug(LL_DEBUG, __LINE__, __FUNCTION__, fmt, ##arg)
#endif
}

#endif
