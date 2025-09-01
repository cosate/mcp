#ifndef MCP_CLIENT_STREAMABLE_HTTP_TRANSPORT_H_
#define MCP_CLIENT_STREAMABLE_HTTP_TRANSPORT_H_


#include <string>
#include "transport.h"
#include "http_client.h"

namespace cosate {
namespace mcp_client {

class StreamableHttpTransport : public Transport {
public:
    StreamableHttpTransport() = default;
    
    explicit StreamableHttpTransport(std::string url, std::shared_ptr<HttpClient> client);

    virtual ~StreamableHttpTransport() = default;

    virtual HttpRequestCompleteInfo SendMessage(nlohmann::json request, std::string sessionId) override;

    virtual void ListenNotification(std::string sessionId) override;

    virtual void SetStop(bool s) override {
        stop_ = s;
    }
    
private:
    std::string mcp_server_url_ = "";

    std::shared_ptr<HttpClient> httpClient_ = nullptr;

    int retryMilliseconds_ = 1;

    bool stop_ = true;

};

}
}

#endif