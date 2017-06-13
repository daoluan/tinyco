#ifndef HTTP_RESPONSE_
#define HTTP_RESPONSE_

#include <string>
#include <tr1/unordered_map>

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
    std::tr1::unordered_map<std::string, std::string>::const_iterator cit =
        headers.find(header);
    return cit != headers.end() ? cit->second : std::string("");
  }

  const std::tr1::unordered_map<std::string, std::string> &GetHeaders() const {
    return headers;
  }

  bool SerializeToString(std::string *output);

  void Clear() {
    status = 200;
    content.clear();
    headers.clear();
  }

 private:
  int status;
  std::string content;
  std::tr1::unordered_map<std::string, std::string> headers;
};
}
}

#endif
