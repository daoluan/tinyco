#ifndef WORK_H_
#define WORK_H_

namespace tinyco {
// wrapper for business
// user can inherit from it and implement Run function
class Work {
 private:
  /* data */
 public:
  Work() {}

  virtual ~Work() {}

  virtual int Run() = 0;
};
}

#endif
