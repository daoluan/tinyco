#ifndef HTTP_RESPONSE_
#define HTTP_RESPONSE_

#include <string>
#include <unordered_map>

namespace tinyco {
namespace http {

class HttpResponse {
 public:
  HttpResponse();
  HttpResponse(const std::string &content, const std::string &content_type,
               int status);
  ~HttpResponse();
  void SetHeader(const std::string &field, const std::string &value);
  void UnsetHeader(const std::string &field);
  void SetStatus(int status);
  void SetContent(const std::string &content);
  int GetStatus() const { return status; }
  std::string GetContent() const { return content; }

  std::string GetHeader(const std::string &header) const {
    std::unordered_map<std::string, std::string>::const_iterator cit =
        headers_.find(header);
    return cit != headers_.end() ? cit->second : std::string("");
  }

  const std::unordered_map<std::string, std::string> &GetHeaders() const {
    return headers_;
  }

  bool SerializeToString(std::string *output) const;
  std::string SerializeToString() const;

  void Clear() {
    status = 200;
    content.clear();
    headers_.clear();
  }

 private:
  int status;
  std::string content;
  std::unordered_map<std::string, std::string> headers_;
};
}
}

#endif
