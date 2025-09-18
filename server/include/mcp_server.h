#ifndef MCP_SERVER_H_
#define MCP_SERVER_H_

#include <variant>
#include <string>
#include <nlohmann/json/json.hpp>
#include "../../common/types.h"

// 引入 Nginx 头，便于在实现中直接使用 ngx_log_error
extern "C" {
    #include <ngx_core.h>
    #include <ngx_http.h>
}

namespace mcp {
namespace server {

class McpServer {
public:
    using MCPRequestVariant = std::variant<
        mcp::InitializeRequest,
        mcp::PingRequest,
        mcp::ListToolsRequest,
        mcp::CallToolRequest,
        mcp::ListResourcesRequest,
        mcp::ListResourceTemplatesRequest,
        mcp::ReadResourceRequest,
        mcp::SubscribeRequest,
        mcp::UnsubscribeRequest,
        mcp::ListPromptsRequest,
        mcp::GetPromptRequest,
        mcp::CompleteRequest1,
        mcp::CompleteRequest2,
        mcp::SetLevelRequest
    >;

    // 仅保留解析函数 (内部直接使用 ngx_log_error)
    static bool ngx_http_mcp_parse_request(const nlohmann::json& j,
                                           const std::string& method,
                                           MCPRequestVariant& out,
                                           ngx_log_t* log);
};

} // namespace server
} // namespace mcp

#endif
