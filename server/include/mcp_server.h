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

    // 新增: 从原始 body 完成 JSON 解析 + method 提取 + 具体类型构建
    static bool parse_body_and_build(const std::string& body,
                                     MCPRequestVariant& out,
                                     std::string& method_out,
                                     ngx_log_t* log);

    // ==== 新增：各类请求处理函数（仅声明，需在 cpp 中实现） ====
    static InitializeResult          handle_initialize(const InitializeRequest&, ngx_log_t* log);
    static EmptyResult               handle_ping(const PingRequest&, ngx_log_t* log);
    static ListToolsResult           handle_list_tools(const ListToolsRequest&, ngx_log_t* log);
    static CallToolResult            handle_call_tool(const CallToolRequest&, ngx_log_t* log);
    static ListResourcesResult       handle_list_resources(const ListResourcesRequest&, ngx_log_t* log);
    static ListResourceTemplatesResult handle_list_resource_templates(const ListResourceTemplatesRequest&, ngx_log_t* log);
    static ReadResourceResult        handle_read_resource(const ReadResourceRequest&, ngx_log_t* log);
    static EmptyResult               handle_subscribe(const SubscribeRequest&, ngx_log_t* log);
    static EmptyResult               handle_unsubscribe(const UnsubscribeRequest&, ngx_log_t* log);
    static ListPromptsResult         handle_list_prompts(const ListPromptsRequest&, ngx_log_t* log);
    static GetPromptResult           handle_get_prompt(const GetPromptRequest&, ngx_log_t* log);
    static CompleteResult            handle_complete_from_resource_template(const CompleteRequest1&, ngx_log_t* log);
    static CompleteResult            handle_complete_from_prompt(const CompleteRequest2&, ngx_log_t* log);
    static EmptyResult               handle_set_level(const SetLevelRequest&, ngx_log_t* log);

    // 统一分发（可在实现里用 std::visit 调用上面函数，再序列化）
    static nlohmann::json            handle(const MCPRequestVariant& req, ngx_log_t* log);

    // 可选工具：将 Result 序列化为 JSON-RPC 响应数据部分
    static nlohmann::json            to_json_result(const Result& r);
};

} // namespace server
} // namespace mcp

#endif
