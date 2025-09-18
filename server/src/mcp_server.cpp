#include "../include/mcp_server.h"
#include <exception>

namespace mcp {
namespace server {

namespace {
enum class MCPMethodId {
    Initialize,
    Ping,
    ToolsList,
    ToolsCall,
    ResourcesList,
    ResourcesTemplatesList,
    ResourcesRead,
    ResourcesSubscribe,
    ResourcesUnsubscribe,
    PromptsList,
    PromptsGet,
    CompleteFromResourceTemplate,
    CompleteFromPrompt,
    LoggingSetLevel,
    Unknown
};

inline MCPMethodId to_method_id(const std::string& m) {
    if (m == "initialize") return MCPMethodId::Initialize;
    if (m == "ping") return MCPMethodId::Ping;
    if (m == "tools/list") return MCPMethodId::ToolsList;
    if (m == "tools/call") return MCPMethodId::ToolsCall;
    if (m == "resources/list") return MCPMethodId::ResourcesList;
    if (m == "resources/templates/list") return MCPMethodId::ResourcesTemplatesList;
    if (m == "resources/read") return MCPMethodId::ResourcesRead;
    if (m == "resources/subscribe") return MCPMethodId::ResourcesSubscribe;
    if (m == "resources/unsubscribe") return MCPMethodId::ResourcesUnsubscribe;
    if (m == "prompts/list") return MCPMethodId::PromptsList;
    if (m == "prompts/get") return MCPMethodId::PromptsGet;
    if (m == "complete/fromResourceTemplate") return MCPMethodId::CompleteFromResourceTemplate;
    if (m == "complete/fromPrompt") return MCPMethodId::CompleteFromPrompt;
    if (m == "logging/setLevel") return MCPMethodId::LoggingSetLevel;
    return MCPMethodId::Unknown;
}
} // namespace

bool McpServer::ngx_http_mcp_parse_request(const nlohmann::json& j,
                                           const std::string& method,
                                           MCPRequestVariant& out,
                                           ngx_log_t* log) {
    try {
        if (method == "initialize")                     out = j.get<mcp::InitializeRequest>();
        else if (method == "ping")                      out = j.get<mcp::PingRequest>();
        else if (method == "tools/list")                out = j.get<mcp::ListToolsRequest>();
        else if (method == "tools/call")                out = j.get<mcp::CallToolRequest>();
        else if (method == "resources/list")            out = j.get<mcp::ListResourcesRequest>();
        else if (method == "resources/templates/list")  out = j.get<mcp::ListResourceTemplatesRequest>();
        else if (method == "resources/read")            out = j.get<mcp::ReadResourceRequest>();
        else if (method == "resources/subscribe")       out = j.get<mcp::SubscribeRequest>();
        else if (method == "resources/unsubscribe")     out = j.get<mcp::UnsubscribeRequest>();
        else if (method == "prompts/list")              out = j.get<mcp::ListPromptsRequest>();
        else if (method == "prompts/get")               out = j.get<mcp::GetPromptRequest>();
        else if (method == "complete/fromResourceTemplate")
                                                         out = j.get<mcp::CompleteRequest1>();
        else if (method == "complete/fromPrompt")       out = j.get<mcp::CompleteRequest2>();
        else if (method == "logging/setLevel")          out = j.get<mcp::SetLevelRequest>();
        else return false;
        return true;
    } catch (const std::exception& e) {
        if (log) ngx_log_error(NGX_LOG_ERR, log, 0,
                               "mcp parse_request std::exception: %s (method=%s)",
                               e.what(), method.c_str());
        return false;
    } catch (...) {
        if (log) ngx_log_error(NGX_LOG_ERR, log, 0,
                               "mcp parse_request unknown exception (method=%s)",
                               method.c_str());
        return false;
    }
}

} // namespace server
} // namespace mcp
