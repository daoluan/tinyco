#include "http_request.h"

#include <sstream>

#define crlf "\r\n"

namespace tinyco {
namespace http {

HttpRequest::HttpRequest()
    : method_(HTTP_REQUEST_METHOD_UNKNOWN), keepalive_(false) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::SetUri(const std::string &uri) { this->struri_ = uri; }

void HttpRequest::SetHeader(const std::string &field,
                            const std::string &value) {
  headers_[field] = value;
}

void HttpRequest::UnsetHeader(const std::string &field) {
  headers_.erase(field);
}

void HttpRequest::SetContent(const std::string &content) {
  this->content_ = content;
  std::stringstream ss;
  ss << content.size();
  headers_["Content-Length"] = ss.str();
}

bool HttpRequest::SerializeToString(std::string *output) const {
  std::stringstream ss;

  // 暂时不支持 GET/POST 以外的其他方法
  switch (method_) {
    case HTTP_REQUEST_METHOD_GET:
      ss << "GET ";
      break;
    case HTTP_REQUEST_METHOD_POST:
      ss << "POST ";
      break;
    default:
      return false;
  }

  ss << struri_ << " ";
  ss << "HTTP/1.1" << crlf;

  std::unordered_map<std::string, std::string>::const_iterator it,
      end = headers_.end();

  for (it = headers_.begin(); it != end; ++it) {
    if (!it->second.empty()) ss << it->first << ": " << it->second << crlf;
  }

  ss << crlf;
  ss << content_;

  *output = ss.str();
  return true;
}

std::string HttpRequest::SerializeToString() const {
  std::string str;
  SerializeToString(&str);
  return str;
}
}
}
