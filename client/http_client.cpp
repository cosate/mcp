#include "http_client.h"
#include <curl/curl.h>
#include <assert.h>

#include "../third_party/spdlog/include/spdlog/spdlog.h"

// extern "C" int Curl_isspace(int c);
// #define ISSPACE(x) (Curl_isspace(static_cast<int>(static_cast<unsigned char>(x))))

#define ISSPACE(c) (c == ' ' || c == '\t' || c == '\r' || c == '\n')


namespace mcp {
namespace client {

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

  spdlog::info("write_cb_sse: " + response->body);
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

HttpClient::HttpClient() {
    CURLcode curlInitStatus = curl_global_init(CURL_GLOBAL_ALL);
    if (curlInitStatus != 0) {
        spdlog::error("curl_global_init failed, error: " + curlInitStatus);
        assert(false);
    }
}

HttpRequestCompleteInfo HttpClient::Get(HttpRequestParam param) {
    HttpRequestCompleteInfo response;

    CURL* curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, param.url.c_str());

    struct curl_slist* headerlist = NULL;
    for (auto item : param.headers) {
        std::string head = item.first + ": " + item.second;
        headerlist = curl_slist_append(headerlist, head.c_str());
    }
    curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerlist);

    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    // curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
  
    curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");

    
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);

    // 重要，mcp get请求param.timeout应当为0，表示永不超时
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, param.timeout);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_sse);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &response);

    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_DEBUGFUNCTION, debug_cb);
    curl_easy_setopt(curl_handle, CURLOPT_DEBUGDATA, &response);

    CURLcode code = curl_easy_perform(curl_handle);
    if (code == 0) {
        long http_status_code = 0;
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_status_code);
        response.code = http_status_code;
        spdlog::info("McpHttpClient::GET, http code: " + http_status_code);
    } else {
        spdlog::info("McpHttpClient::GET, curl error: " + code);
        response.code = code;
    }

    spdlog::info("HttpClient::GET trace: " + response.trace);

    if (headerlist) {
        curl_slist_free_all(headerlist);
    }
    return response;

}

HttpRequestCompleteInfo HttpClient::Post(HttpRequestParam param) {
    HttpRequestCompleteInfo response;

    CURL* curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, param.url.c_str());

    struct curl_slist* headerlist = NULL;
    for (auto item : param.headers) {
        std::string head = item.first + ": " + item.second;
        headerlist = curl_slist_append(headerlist, head.c_str());
    }
    curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerlist);

    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, param.body.c_str());

    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    // curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, param.timeout);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &response);

    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_DEBUGFUNCTION, debug_cb);
    curl_easy_setopt(curl_handle, CURLOPT_DEBUGDATA, &response);


    CURLcode code = curl_easy_perform(curl_handle);
    if (code == 0) {
        long http_status_code = 0;
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_status_code);
        response.code = http_status_code;
        spdlog::info("McpHttpClient::POST, http code: " + http_status_code);
    } else {
        spdlog::info("McpHttpClient::POST, curl error: " + code);
        response.code = code;
    }

    spdlog::info("HttpClient::POST trace: " + response.trace);

    if (headerlist) {
        curl_slist_free_all(headerlist);
    }
    return response;
}

} // namespace client
} // namespace mcp
