#include "http_tool.h"

#include <string>
#include <vector>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <algorithm>

#include "http_parser.h"

namespace tinyco {
namespace http {

template <class T>
struct HttpParserParam {
  T *http_struct;
  std::vector<std::string> headers;
};

// Use template here inorder to reuse function definition.
template <class T>
static int header_field_cb(http_parser *parser, const char *at, size_t length) {
  if (NULL == parser->data) return 0;

  T *t = static_cast<T *>(parser->data);
  t->headers.push_back(std::string(at, length));
  return 0;
}

template <class T>
static int header_value_cb(http_parser *parser, const char *at, size_t length) {
  if (NULL == parser->data) return 0;

  T *t = static_cast<T *>(parser->data);
  t->headers.push_back(std::string(at, length));
  return 0;
}

template <class T>
static int request_url_cb(http_parser *parser, const char *at, size_t length) {
  if (NULL == parser->data) return 0;

  T *t = static_cast<T *>(parser->data);
  t->http_struct->SetUri(std::string(at, length));
  return 0;
}

template <class T>
static int request_body_cb(http_parser *parser, const char *at, size_t length) {
  if (NULL == parser->data) return 0;

  T *t = static_cast<T *>(parser->data);
  t->http_struct->SetContent(std::string(at, length));
  return 0;
}

template <class T>
static int header_complete_cb(http_parser *parser) {
  return 2;
}

int HttpParserImpl::ParseHttpRequest(const std::string &httpbuf,
                                     HttpRequest *http_request) {
  if (!http_request) return 0;

  http_parser_settings settings = {0, 0, 0, 0, 0, 0, 0, 0};
  http_parser parser;

  memset(&parser, 0, sizeof(parser));
  http_parser_init(&parser, HTTP_REQUEST);
  HttpParserParam<HttpRequest> hpp;
  hpp.http_struct = http_request;
  parser.data = &hpp;
  settings.on_header_field = &header_field_cb<HttpParserParam<HttpRequest> >;
  settings.on_header_value = &header_value_cb<HttpParserParam<HttpRequest> >;
  settings.on_url = request_url_cb<HttpParserParam<HttpRequest> >;
  settings.on_body = request_body_cb<HttpParserParam<HttpRequest> >;
  size_t nparsed =
      http_parser_execute(&parser, &settings, httpbuf.data(), httpbuf.size());
  if (nparsed != httpbuf.size()) return -1;

  uint32_t size = hpp.headers.size();
  if ((size & 0x01) != 0) return -2;

  for (uint32_t i = 0; i < size; i += 2) {
    http_request->SetHeader(hpp.headers[i], hpp.headers[i + 1]);
  }

  switch (parser.method) {
    case HTTP_GET:
      http_request->SetMethod(HttpRequest::HTTP_REQUEST_METHOD_GET);
      break;
    case HTTP_POST:
      http_request->SetMethod(HttpRequest::HTTP_REQUEST_METHOD_POST);
      break;
    default:
      http_request->SetMethod(HttpRequest::HTTP_REQUEST_METHOD_UNKNOWN);
      break;
  }

  if (http_request->IsHeader("Connection")) {
    std::string keepalive = http_request->GetHeader("Connection");
    std::transform(keepalive.begin(), keepalive.end(), keepalive.begin(),
                   ::tolower);
    if (keepalive == "keep-alive") http_request->SetKeepAlive(true);
  } else
    // keep alive in default
    http_request->SetKeepAlive(true);

  return 0;
}

int HttpParserImpl::ParseHttpRequestHeadOnly(const std::string &httpbuf,
                                             HttpRequest *http_request,
                                             uint32_t *nparsed) {
  if (!http_request) return 0;

  http_parser_settings settings = {0, 0, 0, 0, 0, 0, 0, 0};
  http_parser parser;

  memset(&parser, 0, sizeof(parser));
  http_parser_init(&parser, HTTP_REQUEST);
  HttpParserParam<HttpRequest> hpp;
  hpp.http_struct = http_request;
  parser.data = &hpp;
  settings.on_header_field = &header_field_cb<HttpParserParam<HttpRequest> >;
  settings.on_header_value = &header_value_cb<HttpParserParam<HttpRequest> >;
  settings.on_url = request_url_cb<HttpParserParam<HttpRequest> >;
  settings.on_headers_complete =
      &header_complete_cb<HttpParserParam<HttpRequest> >;

  size_t n =
      http_parser_execute(&parser, &settings, httpbuf.data(), httpbuf.size());
  if (0 == n) return -1;

  uint32_t size = hpp.headers.size();
  if ((size & 0x01) != 0) return -2;

  for (uint32_t i = 0; i < size; i += 2) {
    http_request->SetHeader(hpp.headers[i], hpp.headers[i + 1]);
  }

  if (nparsed) *nparsed = n;
  return 0;
}

int HttpParserImpl::ParseHttpResponse(const std::string &httpbuf,
                                      HttpResponse *http_response) {
  if (!http_response) return 0;

  http_parser_settings settings = {0, 0, 0, 0, 0, 0, 0, 0};
  http_parser parser;

  memset(&parser, 0, sizeof(parser));
  http_parser_init(&parser, HTTP_RESPONSE);
  HttpParserParam<HttpResponse> hpp;
  hpp.http_struct = http_response;
  parser.data = &hpp;
  settings.on_header_field = header_field_cb<HttpParserParam<HttpResponse> >;
  settings.on_header_value = header_value_cb<HttpParserParam<HttpResponse> >;
  settings.on_body = request_body_cb<HttpParserParam<HttpResponse> >;
  size_t nparsed =
      http_parser_execute(&parser, &settings, httpbuf.data(), httpbuf.size());
  if (nparsed != httpbuf.size()) return -1;

  assert(hpp.headers.size() % 2 == 0);

  uint32_t size = hpp.headers.size();
  for (uint32_t i = 0; i < size; i += 2) {
    http_response->SetHeader(hpp.headers[i], hpp.headers[i + 1]);
  }

  http_response->SetStatus(parser.status_code);
  return 0;
}

// http_parser 辅助函数
static int http_complete(http_parser *parser) {
  *(int *)(parser->data) = 1;
  return -1;  // 返回非零，终止解析
}

int HttpParserImpl::CheckHttp(const std::string &httpbuf) {
  return CheckHttp(httpbuf.c_str(), httpbuf.size(), NULL);
}

int HttpParserImpl::CheckHttp(const void *httpbuf, size_t size) {
  return CheckHttp(httpbuf, size, NULL);
}

int HttpParserImpl::CheckHttp(const std::string &httpbuf, size_t *szparsed) {
  return CheckHttp(httpbuf.c_str(), httpbuf.size(), szparsed);
}

int HttpParserImpl::CheckHttp(const void *httpbuf, size_t size,
                              size_t *szparsed) {
  int complete = 0;
  http_parser_settings settings = {0, 0, 0, 0, 0, 0, 0, 0};
  http_parser parser;
  http_parser_init(&parser, HTTP_BOTH);
  parser.data = &complete;
  settings.on_message_complete = http_complete;

  int nparsed = http_parser_execute(&parser, &settings,
                                    static_cast<const char *>(httpbuf), size);
  if ((static_cast<uint32_t>(nparsed) <= size) && (1 == complete)) {
    if (szparsed) *szparsed = nparsed;
    return HPC_OK;
  } else if (nparsed == 0)  // not http, shutdown
  {
    return HPC_NOT_HTTP;
  } else if ((static_cast<uint32_t>(nparsed) <= size) && (0 == complete)) {
    return HPC_NOT_COMPLETE_HTTP;
  } else {
    return HPC_UNKNOWN;
  }
}

int HttpParserImpl::ParseUrl(const std::string &s, URL *url) {
  return ParseUrl(s.c_str(), s.size(), url);
}

int HttpParserImpl::ParseUrl(const char *s, size_t slen, URL *url) {
  struct http_parser_url hpurl;
  int ret = http_parser_parse_url(s, slen, 0, &hpurl);
  if (ret < 0) return -1;

#define has_and_set(field, member)                           \
  if (hpurl.field_set & (1 << UF_##field))                   \
    url->member.assign(&s[hpurl.field_data[UF_##field].off], \
                       hpurl.field_data[UF_##field].len);

  has_and_set(SCHEMA, schema);
  has_and_set(HOST, host);

  if (hpurl.field_set & (1 << UF_PORT))
    url->port = std::stoul(std::string(&s[hpurl.field_data[UF_PORT].off],
                                       hpurl.field_data[UF_PORT].len).c_str());
  has_and_set(PATH, path);
  has_and_set(QUERY, query);
  has_and_set(FRAGMENT, fragment);
  has_and_set(USERINFO, userinfo);

#undef has_and_set
  return 0;
}
}
}
