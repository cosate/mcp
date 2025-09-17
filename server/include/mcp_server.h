#ifndef MCP_SERVER_H_
#define MCP_SERVER_H_

#include <variant>
#include <string>
#include <nlohmann/json/json.hpp>
#include "../../common/types.h"

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

    static bool ngx_http_mcp_parse_request(const nlohmann::json& j,
                                           const std::string& method,
                                           MCPRequestVariant& out);
};

#endif
