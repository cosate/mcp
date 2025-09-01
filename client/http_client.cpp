#include "http_client.h"
#include "dtfbase/url.h"
#include <curl/curl.h>
#include <assert.h>

// extern "C" int Curl_isspace(int c);
// #define ISSPACE(x) (Curl_isspace(static_cast<int>(static_cast<unsigned char>(x))))

#define ISSPACE(c) (c == ' ' || c == '\t' || c == '\r' || c == '\n')


namespace cosate {
namespace mcp_client {

static size_t write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t sizes = size * nmemb;

  HttpRequestCompleteInfo* response = (HttpRequestCompleteInfo *)userp;
  response->body.append((char*)contents, sizes);
  return sizes;
}

static size_t write_cb_sse(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t sizes = size * nmemb;

  HttpRequestCompleteInfo* response = (HttpRequestCompleteInfo *)userp;
  response->body.append((char*)contents, sizes);

//   FML_LOG(INFO) << "write_cb_sse: " << response->body;
  return sizes;
}

bool ParseRawHeader(const char* buffer, size_t size, std::string& field_name, std::string& field_value) {
  const char* buffer_end = buffer + size;

  const char* colon = buffer;
  while (colon != buffer_end && *colon != ':') {
    ++colon;
  }
  if (colon == buffer || colon == buffer_end) {
    // 冒号是第一个字符，或者没有找到冒号
    return false;
  }

  // 处理name
  {
    const char* name_begin = buffer;
    while (name_begin != colon && ISSPACE(*name_begin)) {
      ++name_begin;
    }
    if (name_begin == colon) {
      // 无有效内容
      return false;
    }
    const char* name_back = colon - 1;
    while (name_back != name_begin && ISSPACE(*name_back)) {
      --name_back;
    }
    if (name_back == name_begin) {
      // 无有效内容
      return false;
    }
    const char* name_end = name_back + 1;
    // HTTP/2规范，headers field name均为小写
    std::transform(name_begin, name_end, std::back_inserter(field_name),
                   [](unsigned char c) { return std::tolower(c); });
  }

  // 处理value
  {
    const char* value_begin = colon + 1;
    while (value_begin != buffer_end && ISSPACE(*value_begin)) {
      ++value_begin;
    }
    if (value_begin == buffer_end) {
      // 无有效内容
      return false;
    }
    const char* value_back = buffer_end - 1;
    while (value_back != value_begin && ISSPACE(*value_back)) {
      --value_back;
    }
    if (value_back == value_begin) {
      // 无有效内容
      return false;
    }
    const char* value_end = value_back + 1;
    field_value = std::string(value_begin, value_end);
  }
  return true;
}


static size_t header_cb(void* buffer, size_t size, size_t nmemb, void* userp) {
    HttpRequestCompleteInfo* response = (HttpRequestCompleteInfo *)userp;
    auto write_size = size * nmemb;

    std::string field_name;
    std::string field_value;
    if (ParseRawHeader((const char*)buffer, write_size, field_name, field_value)) {
      response->headers[field_name] = field_value;
    }
    return size * nmemb;               
}

static size_t debug_cb(CURL*, curl_infotype type, char* data, size_t size, void* user_data) {
      if (auto completeInfo = reinterpret_cast<HttpRequestCompleteInfo*>(user_data)) {
        switch (type) {
          case CURLINFO_DATA_IN:
            // Response body data
            // completeInfo->trace.append("[RESPONSE_BODY] ");
            // completeInfo->trace.append(data, size);
            break;
          case CURLINFO_DATA_OUT:
            // Request body data
            // completeInfo->trace.append("[REQUEST_BODY] ");
            // completeInfo->trace.append(data, size);
            break;
          case CURLINFO_SSL_DATA_IN:
          case CURLINFO_SSL_DATA_OUT:
            break;
          case CURLINFO_TEXT:
          case CURLINFO_HEADER_IN:
          case CURLINFO_HEADER_OUT:
          case CURLINFO_END:
          default:
            completeInfo->trace.append(data, size);
            break;
        }
      }
      return 0;
    };