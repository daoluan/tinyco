#include "http_response.h"

#include <sstream>
#include <stdio.h>

namespace tinyco {
namespace http {

#define crlf "\r\n"

HttpResponse::HttpResponse() { status = 200; }

HttpResponse::HttpResponse(const std::string &content,
                           const std::string &content_type, int status) {
  headers_["content_type"] = content_type;
  this->content = content;
  this->status = status;
}

HttpResponse::~HttpResponse() {}

void HttpResponse::SetHeader(const std::string &field,
                             const std::string &value) {
  headers_[field] = value;
}

void HttpResponse::UnsetHeader(const std::string &field) {
  headers_.erase(field);
}

void HttpResponse::SetStatus(int status) { this->status = status; }

void HttpResponse::SetContent(const std::string &content) {
  this->content = content;
  std::stringstream ss;
  ss << content.size();

  headers_["Content-Length"] = ss.str();
}

bool HttpResponse::SerializeToString(std::string *output) const {
  bool restult = true;
  std::stringstream stream;

  stream << "HTTP/1.1 " << status << " ";
  switch (status) {
    case 200:
      stream << "OK";
      break;
    case 400:
      stream << "Bad Request";
      break;
    case 404:
      stream << "Not Found";
      break;
    case 500:
      stream << "Internal Server Error";
      break;
    default:
      break;
  }
  stream << crlf;

  std::unordered_map<std::string, std::string>::const_iterator it,
      end = headers_.end();
  for (it = headers_.begin(); it != end; ++it) {
    if (!it->second.empty()) stream << it->first << ": " << it->second << crlf;
  }

  stream << crlf;
  stream << content;

  *output = stream.str();
  return restult;
}

std::string HttpResponse::SerializeToString() const {
  std::string str;
  SerializeToString(&str);
  return str;
}
}
}
