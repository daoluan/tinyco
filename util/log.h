
#ifndef LOG_H_
#define LOG_H_

namespace tinyco {
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
}

#endif
