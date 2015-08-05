#ifndef WORK_H__
#define WORK_H__

// 业务代码包装
class Work {
private:
  /* data */
public:
  Work () {
  }

  virtual ~Work () {

  }

  virtual int Run(){
    return 0;
  }
};

#endif
