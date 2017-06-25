#ifndef DEFER_H_
#define DEFER_H_

namespace tinyco {
namespace util {

class defer {
 public:
  defer(std::function<void()> &&t) : t(t) {}
  ~defer() { t(); }

 private:
  std::function<void()> t;
};
}
}

#endif
