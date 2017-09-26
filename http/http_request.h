#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <string>
#include <unordered_map>
#include <inttypes.h>

namespace tinyco {
namespace http {

struct URL {
  std::string schema;
  std::string host;
  uint32_t port;
  std::string path;
  std::string query;
  std::string fragment;
  std::string userinfo;
  std::unordered_map<std::string, std::string> query_params;

  bool IsHttps() const { return 443 == port; }

  bool IsSet(const std::string &param) {
    return query_params.find(param) != query_params.end();
  }
};

class HttpRequest {
 public:
  HttpRequest();
  ~HttpRequest();
  void SetMethod(uint8_t method) { method_ = method; }
  void SetUri(const std::string &url);
  void SetHeader(const std::string &field, const std::string &value);
  int GetMethod() const { return method_; }
  std::string GetUri() const { return struri_; }
  void SetKeepAlive(bool b) { keepalive_ = b; }
  bool KeepAlive() const { return keepalive_; }
  URL &GetUriObj() { return uri_; }

  std::string GetHeader(const std::string &header) const {
    std::unordered_map<std::string, std::string>::const_iterator cit =
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
  std::string SerializeToString() const;

  void Clear() {
    method_ = HTTP_REQUEST_METHOD_UNKNOWN;
    struri_.clear();
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
  std::string struri_;
  URL uri_;
  std::string content_;
  std::unordered_map<std::string, std::string> headers_;
  bool keepalive_;
};
}
}

#endif
