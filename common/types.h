#ifndef MCP_CLIENT_TYPES_H_
#define MCP_CLIENT_TYPES_H_

/** 
 * https://github.com/modelcontextprotocol/modelcontextprotocol/blob/main/schema/draft/schema.json
 */

#include <optional>
#include <variant>
#include <vector>
#include <map>
#include "../third_party/nlohmann/json/json.hpp"

namespace nlohmann {
    template <typename T>
    void to_json(json& j, const std::optional<T>& opt) {
        if (opt.has_value()) {
            j = opt.value();
        } else {
            j = nullptr;
        }
    }

    template <typename T>
    void from_json(const json& j, std::optional<T>& opt) {
        if (j.is_null() || j.is_discarded()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>();
        }
    }
}

// Helper macro to check if a type is std::optional
#define NLOHMANN_JSON_IS_OPTIONAL(Type) \
    std::is_same_v<Type, std::optional<typename Type::value_type>>

// Helper function to check if a type is std::optional
template<typename T>
struct is_optional : std::false_type {};

template<typename T>
struct is_optional<std::optional<T>> : std::true_type {};

// Helper functions to handle serialization
template<typename T>
void assign_json_value(nlohmann::json& j, const char* key, const T& value, std::true_type) {
    // T is std::optional
    if (value.has_value()) {
        j[key] = value.value();
    }
}

template<typename T>
void assign_json_value(nlohmann::json& j, const char* key, const T& value, std::false_type) {
    // T is not std::optional
    j[key] = value;
}

template<typename T>
void assign_json_value(nlohmann::json& j, const char* key, const T& value) {
    assign_json_value(j, key, value, is_optional<std::decay_t<T>>{});
}

// Custom macro to skip std::nullopt values during serialization
#define NLOHMANN_JSON_TO_OPTIONAL(v1) \
    assign_json_value(nlohmann_json_j, #v1, nlohmann_json_t.v1);

// Custom macro to handle optional fields during deserialization
#define NLOHMANN_JSON_FROM_OPTIONAL(v1) \
    nlohmann_json_t.v1 = nlohmann_json_j.value(#v1, nlohmann_json_t.v1);

#define NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Type, ...)  \
    friend void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) { \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO_OPTIONAL, __VA_ARGS__)) \
    } \
    friend void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) { \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_OPTIONAL, __VA_ARGS__)) \
    }

namespace mcp {

struct BaseMetaData {
    virtual ~BaseMetaData() = default;

    std::string name;
    std::optional<std::string> title = std::nullopt;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(BaseMetaData, name, title)
};

struct Implementation : public BaseMetaData {
    Implementation() : BaseMetaData() {
        name = "mcp";
        version = "0.1.0";
    }

    std::string version;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Implementation, name, title, version)

};

struct Result {
    virtual ~Result() = default;
    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Result, _meta);
};

struct PaginatedResult : public Result {
    virtual ~PaginatedResult() = default;

    std::optional<std::string> nextCursor;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(PaginatedResult, _meta, nextCursor)
};


struct EmptyResult : public Result {

};

struct PromptArgument : public BaseMetaData {
    std::optional<std::string> description;
    std::optional<bool> required;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(PromptArgument, name, title, description, required)
};


struct Prompt : public BaseMetaData {
    std::optional<std::string> description;
    std::optional<std::vector<PromptArgument> > arguments;

    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Prompt, name, title, description, arguments, _meta)
};

struct ToolAnnotations {
    std::optional<std::string> title;
    std::optional<bool> readOnlyHint;
    std::optional<bool> destructiveHint;
    std::optional<bool> idempotentHint;
    std::optional<bool> openWorldHint;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ToolAnnotations, title, readOnlyHint, destructiveHint, idempotentHint, openWorldHint)
};

struct Tool : public BaseMetaData {
    std::optional<std::string> description;
    nlohmann::json inputSchema;
    std::optional<nlohmann::json> outputSchema;
    std::optional<ToolAnnotations> annotations;

    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Tool, name, title, description, inputSchema, outputSchema, annotations, _meta)

};

struct PromptCapability {
    std::optional<bool> listChanged;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(PromptCapability, listChanged)
};

struct ResourceCapability {
    std::optional<bool> subscribe;
    std::optional<bool> listChanged;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ResourceCapability, subscribe, listChanged)
};

struct ToolsCapability {
    std::optional<bool> listChanged;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ToolsCapability, listChanged)
};

struct ServerCapabilities {
    std::optional<nlohmann::json> experimental;
    std::optional<nlohmann::json> logging;
    std::optional<PromptCapability> prompts;
    std::optional<ResourceCapability> resources;
    std::optional<ToolsCapability> tools;
    std::optional<nlohmann::json> completions;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ServerCapabilities, experimental, logging, prompts, resources, tools, completions)
    
};

struct InitializeResult : public Result {
    std::string protocolVersion;
    Implementation serverInfo;
    ServerCapabilities capabilities;
    std::optional<std::string> instructions;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(InitializeResult, _meta, protocolVersion, serverInfo, capabilities, instructions)
};


struct ListToolsResult : public PaginatedResult {
    std::vector<Tool> tools;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ListToolsResult, _meta, nextCursor, tools)
};

struct Annotations {
    std::optional<std::vector<std::string> > audience;
    std::optional<std::string> lastModified;
    double priority;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Annotations, audience, lastModified, priority)
};

struct Content {
    virtual ~Content() = default;
    std::string type;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Content, type)
};

struct TextContent : public Content {
    TextContent() {
        type = "text";
    }

    std::string text;
    std::optional<Annotations> annotations;
    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(TextContent, type, text, annotations, _meta)
};

struct ImageContent : public Content {
    ImageContent() {
        type = "image";
    }

    std::string data;
    std::string mimeType;
    std::optional<Annotations> annotations;
    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ImageContent, type, data, mimeType, annotations, _meta)
};

struct AudioContent : public Content {
    AudioContent() {
        type = "audio";
    }

    std::string data;
    std::string mimeType;
    std::optional<Annotations> annotations;
    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(AudioContent, type, data, mimeType, annotations, _meta)
};

struct ResourceLink : public Content {
    ResourceLink() {
        type = "resource_link";
    }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ResourceLink, type)
};

// todo: url
struct ResourceContents {
    virtual ~ResourceContents() = default;
    std::string url;
    std::optional<std::string> mimeType;
    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ResourceContents, url, mimeType, _meta)
};

struct TextResourceContents : public ResourceContents {
    std::string text;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(TextResourceContents, url, mimeType, _meta, text)
};

struct BlobResourceContents : public ResourceContents {
    std::string blob;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(BlobResourceContents, url, mimeType, _meta, blob)
};

struct EmbeddedResource : public Content {
    EmbeddedResource() {
        type = "resource";
    }

    ResourceContents resource;
    std::optional<Annotations> annotations;
    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(EmbeddedResource, type, resource, annotations, _meta)
};

// todo: 
struct CallToolResult : public Result {
    bool isError = false;
    std::vector<Content> content;
    std::optional<nlohmann::json> structureContent;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CallToolResult, _meta, isError, content, structureContent)
};

struct ResourceTemplate : public BaseMetaData {
    std::string uriTemplate;
    std::optional<std::string> description;
    std::optional<std::string> mimeType;
    std::optional<Annotations> annotations;
    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ResourceTemplate, name, title, uriTemplate, description, mimeType, annotations, _meta)
};

struct Resource : public BaseMetaData {
    std::string uri;
    std::optional<int> size;
    std::optional<std::string> description;
    std::optional<std::string> mimeType;
    std::optional<Annotations> annotations;
    std::optional<nlohmann::json> _meta;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Resource, name, title, uri, size, description, mimeType, annotations, _meta)
};

struct ListResourcesResult : public PaginatedResult {
    std::vector<Resource> resources;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ListResourcesResult, _meta, nextCursor, resources)
};


struct ListResourceTemplatesResult : public PaginatedResult {
    std::vector<ResourceTemplate> resourceTemplates;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ListResourceTemplatesResult, _meta, nextCursor, resourceTemplates)
};

//todo: content
struct ReadResourceResult : public Result {
    std::vector<nlohmann::json> contents;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ReadResourceResult, _meta, contents)
};

struct ListPromptsResult : public PaginatedResult {
    std::vector<Prompt> prompts;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ListPromptsResult, _meta, nextCursor, prompts)
};


// todo: content
struct PromptMessage {
    std::string role;
    Content content;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(PromptMessage, role, content)
};

struct GetPromptResult : public Result {
    std::optional<std::string> description;
    std::optional<std::vector<PromptMessage> > messages;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(GetPromptResult, _meta, description, messages)
};

struct Completion {
    std::optional<bool> hasMore;
    std::optional<int> total;
    std::vector<std::string> values;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Completion, hasMore, total, values)
};

struct CompleteResult : public Result {
    Completion completion;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CompleteResult, _meta, completion)
};

struct ResourceTemplateReference {
    std::string type;
    std::string uri;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ResourceTemplateReference, type, uri)
};

struct PromptReference {
    std::string type;
    std::string name;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(PromptReference, type, name)
};

struct RootsCapability {
    std::optional<bool> listChanged;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(RootsCapability, listChanged)
};

struct ClientCapabilities {
    std::optional<nlohmann::json> experimental = std::nullopt;
    std::optional<nlohmann::json> elicitation = std::nullopt;
    std::optional<RootsCapability> roots = std::nullopt;
    std::optional<nlohmann::json> sampling = std::nullopt;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ClientCapabilities, experimental, elicitation, roots, sampling)
};

struct ClientRequest {
    virtual ~ClientRequest() = default;
    std::string method;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ClientRequest, method)
};

struct RequestParams {
    virtual ~RequestParams() = default;
    struct Meta {
        std::optional<std::string> progressToken;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Meta, progressToken);
    };

    std::optional<Meta> _meta = std::nullopt;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(RequestParams, _meta)
};

struct InitializeRequestParams : public RequestParams {
    std::string protocolVersion;
    ClientCapabilities capabilities;
    Implementation clientInfo;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(InitializeRequestParams, _meta, protocolVersion, capabilities, clientInfo)
};

struct InitializeRequest : public ClientRequest {
    InitializeRequestParams params;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(InitializeRequest, method, params)
};

struct PingRequest : public ClientRequest {
    std::optional<nlohmann::json> params;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(PingRequest, method, params)
};


struct PaginatedRequestParams : public RequestParams {
    std::optional<std::string> cursor;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(PaginatedRequestParams, _meta, cursor)
};

struct PaginatedRequest : public ClientRequest {
    std::optional<PaginatedRequestParams> params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(PaginatedRequest, method, params)
};

struct ListToolsRequest : public PaginatedRequest {
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ListToolsRequest, method, params)
};

struct CallToolRequestParams : public RequestParams {
    std::string name;
    std::map<std::string, std::string> arguments;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CallToolRequestParams, _meta, name, arguments)
};

struct CallToolRequest : public ClientRequest {
    CallToolRequestParams params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CallToolRequest, method, params)
};

struct ListResourcesRequest : public PaginatedRequest {
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ListResourcesRequest, method, params)
};

struct ListResourceTemplatesRequest : public PaginatedRequest {
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ListResourceTemplatesRequest, method, params)
};

struct ReadResourceRequestParams : public RequestParams {
    std::string uri;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ReadResourceRequestParams, _meta, uri)
};

struct ReadResourceRequest : public ClientRequest {
    ReadResourceRequestParams params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ReadResourceRequest, method, params)
};

struct SubscribeRequestParams : public RequestParams {
    std::string uri;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(SubscribeRequestParams, _meta, uri)
};

struct SubscribeRequest : public ClientRequest {
    SubscribeRequestParams params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(SubscribeRequest, method, params)
};

struct UnsubscribeRequestParams : public RequestParams {
    std::string uri;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(UnsubscribeRequestParams, _meta, uri)
};

struct UnsubscribeRequest : public ClientRequest {
    UnsubscribeRequestParams params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(UnsubscribeRequest, method, params)
};

struct ListPromptsRequest : public PaginatedRequest {
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(ListPromptsRequest, method, params)
};

struct GetPromptRequestParams : public RequestParams {
    std::string name;
    std::map<std::string, std::string> arguments;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(GetPromptRequestParams, _meta, name, arguments)
};

struct GetPromptRequest : public ClientRequest {
    GetPromptRequestParams params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(GetPromptRequest, method, params)
};

struct CompletionArgument {
    virtual ~CompletionArgument() = default;
    std::string name;
    std::string value;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CompletionArgument, name, value)
};

struct CompletionContext {
    virtual ~CompletionContext() = default;
    std::optional<std::map<std::string, std::string> > arguments;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CompletionContext, arguments)
};

struct CompleteRequestParams1 : public RequestParams {
    virtual ~CompleteRequestParams1() = default;
    ResourceTemplateReference ref;
    CompletionArgument argument;
    std::optional<CompletionContext> context;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CompleteRequestParams1, _meta, ref, argument, context)
};

struct CompleteRequestParams2 : public RequestParams {
    virtual ~CompleteRequestParams2() = default;
    PromptReference ref;
    CompletionArgument argument;
    std::optional<CompletionContext> context;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CompleteRequestParams2, _meta, ref, argument, context)
};

struct CompleteRequest1 : public ClientRequest {
    CompleteRequestParams1 params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CompleteRequest1, method, params)
};

struct CompleteRequest2 : public ClientRequest {
    CompleteRequestParams2 params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(CompleteRequest2, method, params)
};

struct SetLevelRequestParams : public RequestParams {
    std::string level;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(SetLevelRequestParams, level)
};

struct SetLevelRequest : public ClientRequest {
    SetLevelRequestParams params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(SetLevelRequest, method, params)
};


struct Notification {
    virtual ~Notification() = default;
    std::string method;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(Notification, method)
};

struct InitializedNotification : public Notification {

};

struct JSONRPCNotification {
    std::string jsonrpc = "2.0";
    std::optional<nlohmann::json> params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(JSONRPCNotification, jsonrpc, params)
};

struct JSONRPCRequest {
    std::string jsonrpc = "2.0";
    std::string method;
    std::string id;
    std::optional<nlohmann::json> params;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(JSONRPCRequest, jsonrpc, method, id, params)
};

struct JSONRPCResponse {
    std::string jsonrpc = "2.0";
    std::string id;
    nlohmann::json result;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_OPTIONAL(JSONRPCResponse, jsonrpc, id, result)
};


}

#endif
