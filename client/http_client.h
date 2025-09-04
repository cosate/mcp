#ifndef MCP_CLIENT_HTTP_CLIENT_H_
#define MCP_CLIENT_HTTP_CLIENT_H_

#include <functional>
#include <map>
#include <string>

namespace mcp {
namespace client {

struct HttpRequestParam {
  /**
   * @brief 请求URL
   */
  std::string url;

  /**
   * @brief 请求方法
   * @note 为空时根据body设置默认值。body为nullptr时使用GET，有值时（即使Size为0）使用POST
   * @note method是HTTP协议中为数不多的对大小写敏感的字段。自定义时务必注意大小写。
   * @sa body
   */
  std::string method;

  /**
   * @brief 请求Headers
   * @note 该字段会和全局设置的Headers合并
   * @note key相同时使用HttpRequestParam::headers中对应的value
   * @note 将value设置为空字符串时可以移除对应key的header
   */
  std::map<std::string, std::string> headers;

  /*
   * @brief 请求体
   * @note 该字段会影响对method默认值的判断
   * @sa method
   */
  std::string body;

  int timeout = 5000; // 5000ms
};

/**
 * @brief HTTP请求进度信息
 */
struct HttpRequestProgressInfo {
  int64_t request_id = 0;
  int64_t upload_current_bytes = 0;
  int64_t upload_total_bytes = 0;
  int64_t download_current_bytes = 0;
  int64_t download_total_bytes = 0;
};
using HttpRequestProgressCallback = std::function<bool(const HttpRequestProgressInfo& info)>;


struct HttpRequestCompleteInfo{
      int code;
      std::string error_message;
      std::map<std::string, std::string> headers;
      std::string body;

      std::string trace;
};
using HttpRequestCompleteCallback = std::function<void(HttpRequestCompleteInfo info)>;

class HttpClient {
public: 
    explicit HttpClient();

    virtual HttpRequestCompleteInfo Get(HttpRequestParam param);

    virtual HttpRequestCompleteInfo Post(HttpRequestParam param);

};

}
}

#endif
