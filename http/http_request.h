#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <string>
#include <tr1/unordered_map>
#include <inttypes.h>

namespace tinyco {
namespace http {

class HttpRequest {
 public:
  HttpRequest();
  ~HttpRequest();
  void SetMethod(uint8_t method) { method_ = method; }
  void SetUri(const std::string &url);
  void SetHeader(const std::string &field, const std::string &value);
  int GetMethod() const { return method_; }
  std::string GetUrl() const { return uri_; }

  std::string GetHeader(const std::string &header) const {
    std::tr1::unordered_map<std::string, std::string>::const_iterator cit =
        headers_.find(header);

    return cit != headers_.end() ? cit->second : std::string("");
  }

  std::string GetContent() const { return content_; }
  void UnsetHeader(const std::string &field);
  void SetContent(const std::string &content);

  bool IsHeader(const std::string &header) const {
    return headers_.find(header) != headers_.end();
  }

  bool SerializeToString(std::string *output) const;

  void Clear() {
    method_ = HTTP_REQUEST_METHOD_UNKNOWN;
    uri_.clear();
    content_.clear();
    headers_.clear();
  }

  enum {
    HTTP_REQUEST_METHOD_UNKNOWN = -1,
    HTTP_REQUEST_METHOD_GET = 0,
    HTTP_REQUEST_METHOD_POST,
  };

 private:
  uint8_t method_;
  std::string uri_;
  std::string content_;
  std::tr1::unordered_map<std::string, std::string> headers_;
};
}
}

#endif
