#include "../include/mcp_server.h"

bool McpServer::ngx_http_mcp_parse_request(const nlohmann::json& j,
                                           const std::string& method,
                                           MCPRequestVariant& out) {
    try {
        if (method == "initialize")                  out = j.get<mcp::InitializeRequest>();
        else if (method == "ping")                   out = j.get<mcp::PingRequest>();
        else if (method == "tools/list")             out = j.get<mcp::ListToolsRequest>();
        else if (method == "tools/call")             out = j.get<mcp::CallToolRequest>();
        else if (method == "resources/list")         out = j.get<mcp::ListResourcesRequest>();
        else if (method == "resources/templates/list") out = j.get<mcp::ListResourceTemplatesRequest>();
        else if (method == "resources/read")         out = j.get<mcp::ReadResourceRequest>();
        else if (method == "resources/subscribe")    out = j.get<mcp::SubscribeRequest>();
        else if (method == "resources/unsubscribe")  out = j.get<mcp::UnsubscribeRequest>();
        else if (method == "prompts/list")           out = j.get<mcp::ListPromptsRequest>();
        else if (method == "prompts/get")            out = j.get<mcp::GetPromptRequest>();
        else if (method == "complete/fromResourceTemplate")
                                                     out = j.get<mcp::CompleteRequest1>();
        else if (method == "complete/fromPrompt")    out = j.get<mcp::CompleteRequest2>();
        else if (method == "logging/setLevel")       out = j.get<mcp::SetLevelRequest>();
        else return false;
        return true;
    } catch (...) {
        return false;
    }
}
