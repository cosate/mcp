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

bool McpServer::parse_body_and_build(const std::string& body,
                                     MCPRequestVariant& out,
                                     std::string& method_out,
                                     ngx_log_t* log) {
    if (body.empty()) {
        if (log) ngx_log_error(NGX_LOG_ERR, log, 0, "mcp empty request body");
        return false;
    }
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (const std::exception& e) {
        if (log) ngx_log_error(NGX_LOG_ERR, log, 0,
                               "mcp json parse_body_and_build error: %s", e.what());
        return false;
    } catch (...) {
        if (log) ngx_log_error(NGX_LOG_ERR, log, 0,
                               "mcp json parse_body_and_build unknown error");
        return false;
    }
    auto it = j.find("method");
    if (it == j.end() || !it->is_string()) {
        if (log) ngx_log_error(NGX_LOG_ERR, log, 0,
                               "mcp missing or non-string 'method'");
        return false;
    }
    method_out = it->get<std::string>();
    if (!ngx_http_mcp_parse_request(j, method_out, out, log)) {
        if (log) ngx_log_error(NGX_LOG_ERR, log, 0,
                               "mcp unsupported method=%s", method_out.c_str());
        return false;
    }
    return true;
}

// ====== 新增: 各具体请求处理实现 ======
InitializeResult McpServer::handle_initialize(const InitializeRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_initialize");
    InitializeResult r;
    r.protocolVersion = req.params.protocolVersion.empty() ? "1.0" : req.params.protocolVersion;
    r.serverInfo = Implementation(); // 默认构造
    // 可根据 req.params.capabilities 设置 r.capabilities
    return r;
}

EmptyResult McpServer::handle_ping(const PingRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_ping");
    return EmptyResult{};
}

ListToolsResult McpServer::handle_list_tools(const ListToolsRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_list_tools");
    ListToolsResult r;
    r.tools = {}; // 返回空列表，占位
    return r;
}

CallToolResult McpServer::handle_call_tool(const CallToolRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_call_tool name=%s",
                           req.params.name.c_str());
    CallToolResult r;
    r.isError = false;
    // 简单回显
    nlohmann::json echo;
    echo["called"] = req.params.name;
    echo["args"] = req.params.arguments;
    r.structureContent = echo;
    return r;
}

ListResourcesResult McpServer::handle_list_resources(const ListResourcesRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_list_resources");
    ListResourcesResult r;
    r.resources = {};
    return r;
}

ListResourceTemplatesResult McpServer::handle_list_resource_templates(
        const ListResourceTemplatesRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_list_resource_templates");
    ListResourceTemplatesResult r;
    r.resourceTemplates = {};
    return r;
}

ReadResourceResult McpServer::handle_read_resource(const ReadResourceRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_read_resource uri=%s",
                           req.params.uri.c_str());
    ReadResourceResult r;
    nlohmann::json item;
    item["uri"] = req.params.uri;
    item["content"] = "placeholder";
    r.contents.push_back(item);
    return r;
}

EmptyResult McpServer::handle_subscribe(const SubscribeRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_subscribe uri=%s",
                           req.params.uri.c_str());
    return EmptyResult{};
}

EmptyResult McpServer::handle_unsubscribe(const UnsubscribeRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_unsubscribe uri=%s",
                           req.params.uri.c_str());
    return EmptyResult{};
}

ListPromptsResult McpServer::handle_list_prompts(const ListPromptsRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_list_prompts");
    ListPromptsResult r;
    r.prompts = {};
    return r;
}

GetPromptResult McpServer::handle_get_prompt(const GetPromptRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0, "mcp handle_get_prompt name=%s",
                           req.params.name.c_str());
    GetPromptResult r;
    r.description = std::string("Prompt: ") + req.params.name;
    return r;
}

CompleteResult McpServer::handle_complete_from_resource_template(
        const CompleteRequest1& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0,
                           "mcp handle_complete_from_resource_template ref=%s",
                           req.params.ref.uri.c_str());
    CompleteResult r;
    r.completion.values.push_back("completion-from-resource-template");
    return r;
}

CompleteResult McpServer::handle_complete_from_prompt(
        const CompleteRequest2& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0,
                           "mcp handle_complete_from_prompt name=%s",
                           req.params.ref.name.c_str());
    CompleteResult r;
    r.completion.values.push_back("completion-from-prompt");
    return r;
}

EmptyResult McpServer::handle_set_level(const SetLevelRequest& req, ngx_log_t* log) {
    if (log) ngx_log_error(NGX_LOG_INFO, log, 0,
                           "mcp handle_set_level level=%s", req.params.level.c_str());
    return EmptyResult{};
}

// 将 Result 系列对象简单转 json（借助已生成的 to_json）
nlohmann::json McpServer::to_json_result(const Result& r) {
    nlohmann::json j = r; // 触发 ADL 的 to_json
    return j;
}

// 统一分发：返回可直接用作 JSON-RPC result 字段的对象
nlohmann::json McpServer::handle(const MCPRequestVariant& req, ngx_log_t* log) {
    return std::visit([log](auto const& concrete) -> nlohmann::json {
        using T = std::decay_t<decltype(concrete)>;
        if constexpr (std::is_same_v<T, InitializeRequest>) {
            return to_json_result(handle_initialize(concrete, log));
        } else if constexpr (std::is_same_v<T, PingRequest>) {
            return to_json_result(handle_ping(concrete, log));
        } else if constexpr (std::is_same_v<T, ListToolsRequest>) {
            return to_json_result(handle_list_tools(concrete, log));
        } else if constexpr (std::is_same_v<T, CallToolRequest>) {
            return to_json_result(handle_call_tool(concrete, log));
        } else if constexpr (std::is_same_v<T, ListResourcesRequest>) {
            return to_json_result(handle_list_resources(concrete, log));
        } else if constexpr (std::is_same_v<T, ListResourceTemplatesRequest>) {
            return to_json_result(handle_list_resource_templates(concrete, log));
        } else if constexpr (std::is_same_v<T, ReadResourceRequest>) {
            return to_json_result(handle_read_resource(concrete, log));
        } else if constexpr (std::is_same_v<T, SubscribeRequest>) {
            return to_json_result(handle_subscribe(concrete, log));
        } else if constexpr (std::is_same_v<T, UnsubscribeRequest>) {
            return to_json_result(handle_unsubscribe(concrete, log));
        } else if constexpr (std::is_same_v<T, ListPromptsRequest>) {
            return to_json_result(handle_list_prompts(concrete, log));
        } else if constexpr (std::is_same_v<T, GetPromptRequest>) {
            return to_json_result(handle_get_prompt(concrete, log));
        } else if constexpr (std::is_same_v<T, CompleteRequest1>) {
            return to_json_result(handle_complete_from_resource_template(concrete, log));
        } else if constexpr (std::is_same_v<T, CompleteRequest2>) {
            return to_json_result(handle_complete_from_prompt(concrete, log));
        } else if constexpr (std::is_same_v<T, SetLevelRequest>) {
            return to_json_result(handle_set_level(concrete, log));
        } else {
            // 理论不会到此
            nlohmann::json err;
            err["error"] = "unhandled request variant";
            return err;
        }
    }, req);
}

} // namespace server
} // namespace mcp
