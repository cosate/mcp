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
        switch (to_method_id(method)) {
            case MCPMethodId::Initialize:
                out = j.get<mcp::InitializeRequest>();
                break;
            case MCPMethodId::Ping:
                out = j.get<mcp::PingRequest>();
                break;
            case MCPMethodId::ToolsList:
                out = j.get<mcp::ListToolsRequest>();
                break;
            case MCPMethodId::ToolsCall:
                out = j.get<mcp::CallToolRequest>();
                break;
            case MCPMethodId::ResourcesList:
                out = j.get<mcp::ListResourcesRequest>();
                break;
            case MCPMethodId::ResourcesTemplatesList:
                out = j.get<mcp::ListResourceTemplatesRequest>();
                break;
            case MCPMethodId::ResourcesRead:
                out = j.get<mcp::ReadResourceRequest>();
                break;
            case MCPMethodId::ResourcesSubscribe:
                out = j.get<mcp::SubscribeRequest>();
                break;
            case MCPMethodId::ResourcesUnsubscribe:
                out = j.get<mcp::UnsubscribeRequest>();
                break;
            case MCPMethodId::PromptsList:
                out = j.get<mcp::ListPromptsRequest>();
                break;
            case MCPMethodId::PromptsGet:
                out = j.get<mcp::GetPromptRequest>();
                break;
            case MCPMethodId::CompleteFromResourceTemplate:
                out = j.get<mcp::CompleteRequest1>();
                break;
            case MCPMethodId::CompleteFromPrompt:
                out = j.get<mcp::CompleteRequest2>();
                break;
            case MCPMethodId::LoggingSetLevel:
                out = j.get<mcp::SetLevelRequest>();
                break;
            case MCPMethodId::Unknown:
            default:
                return false;
        }
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
