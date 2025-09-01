#include <future>
#include <chrono>

#include "streamable_http_transport.h"

namespace cosate {
namespace mcp_client {

StreamableHttpTransport::StreamableHttpTransport(std::string url, std::shared_ptr<HttpClient> client) : Transport() {
    mcp_server_url_ = url;
    httpClient_ = client;
}

HttpRequestCompleteInfo StreamableHttpTransport::SendMessage(nlohmann::json request, std::string sessionId) {
    if (!httpClient_) {
        // FML_LOG(ERROR) << "StreamableHttpTransport::SendMessage, httpClient null";
        return {};
    }

    HttpRequestParam param;
    param.url = mcp_server_url_;

    std::map<std::string, std::string> headers;
    headers["Accept"] = "application/json, text/event-stream";
    headers["Content-Type"] = "application/json";
    if (!sessionId.empty()) {
        headers["mcp-session-id"] = sessionId;
    }
    // headers["User-Agent"] = "curl/8.7.1";

    param.headers = headers;
    param.method = "POST";
    param.body = request.dump();

    // FML_LOG(INFO) << "fanka_sse_send: " << param.body;

    HttpRequestCompleteInfo response = httpClient_->Post(param);

    return response;
}

void StreamableHttpTransport::ListenNotification(std::string sessionId) {
    if (!httpClient_) {
        // FML_LOG(ERROR) << "StreamableHttpTransport::ListenNotification, httpClient null";
        return;
    }
    HttpRequestParam param;
    param.url = mcp_server_url_;

    std::map<std::string, std::string> headers;
    headers["Accept"] = "application/json, text/event-stream";
    if (!sessionId.empty()) {
        headers["mcp-session-id"] = sessionId;
    }

    param.url = mcp_server_url_;
    param.headers = headers;
    param.method = "GET";

    while (!stop_) {
        auto response = httpClient_->Get(param);

        // FML_LOG(INFO) << "StreamableHttpTransport::ListenNotification get return";

        // 指数退避
        std::this_thread::sleep_for(std::chrono::milliseconds(retryMilliseconds_));
        retryMilliseconds_ = retryMilliseconds_ * 2;
        if (retryMilliseconds_ > 1000) {
            break;
        }
    }

    // FML_LOG(INFO) << "StreamableHttpTransport::ListenNotification end";
}

}
}