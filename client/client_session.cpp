#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>

#include "client_session.h"
#include "../third_party/spdlog/include/spdlog/spdlog.h"

namespace mcp {
namespace client {

#define MCP_VERSION "2025-06-18";

ClientSession::ClientSession(std::shared_ptr<Transport> transport, Implementation clientInfo): request_id_counter_(0) {
    transport_ = transport;
    clientInfo_ = clientInfo;
    RootsCapability root;
    root.listChanged = true;
    clientCapabilities_.roots = root;
}

ClientSession::~ClientSession() {
    if (listenThread_) {
        transport_->SetStop(true);
    }
}

void ClientSession::SetClientCapabilities() {
    
}

std::string ClientSession::GenerateReuqestId() {
    // Generate a unique ID using a combination of:
    // - Incrementing counter
    // - Random component
    // - timestamp

    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<> dist(0, 9999);

    uint64_t counter = request_id_counter_++;
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    int random = dist(rng);

    std::stringstream ss;
    ss << std::hex << timestamp << "-" << counter << "-" << random;
    return ss.str();
}

InitializeResult ClientSession::Initialize() {
    auto request = std::make_shared<InitializeRequest>();
    request->method = "initialize";
    request->params.clientInfo = clientInfo_;
    request->params.protocolVersion = MCP_VERSION;
    request->params.capabilities.elicitation = clientCapabilities_.elicitation;
    request->params.capabilities.experimental = std::nullopt;
    request->params.capabilities.sampling = clientCapabilities_.sampling;
    request->params.capabilities.roots = clientCapabilities_.roots;

    auto sseResponse = SendRequest(request);

    spdlog::info("sse: {}", sseResponse.back().data.dump());
    try {
        if (sseResponse.size() == 0) {
            return InitializeResult();
        }
        auto response = sseResponse.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<InitializeResult>();
        
        serverCapabilities_ = ret.capabilities;

        nlohmann::json noti = nlohmann::json::object();
        noti["method"] = "notifications/initialized";
        SendNotification(noti);

        // StartSSElisten();

        std::this_thread::sleep_for(std::chrono::milliseconds(10000));

        auto tools = ListTools();
        spdlog::info("list_tools : {}", nlohmann::json(tools).dump());

        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return InitializeResult();
    }
}

EmptyResult ClientSession::SendPing() {
    auto request = std::make_shared<PingRequest>();
    request->method = "ping";

    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<EmptyResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return EmptyResult();
    }

    return EmptyResult();
}

ListToolsResult ClientSession::ListTools(const std::string& cursor) {
    auto request = std::make_shared<ListToolsRequest>();
    request->method = "tools/list";
    request->params->cursor = cursor;
    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<ListToolsResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return ListToolsResult();
    }

    return ListToolsResult();

}

CallToolResult ClientSession::CallTool(const std::string& name, 
                        const std::map<std::string, std::string>& arguments) {
    auto request = std::make_shared<CallToolRequest>();
    request->method = "tools/call";
    request->params.name = name;
    request->params.arguments = arguments;

    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<CallToolResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return CallToolResult();
    }

    return CallToolResult();
    
}

ListResourcesResult ClientSession::ListResources(const std::string& cursor) {
    auto request = std::make_shared<ListResourcesRequest>();
    request->method = "resources/list";
    request->params->cursor = cursor;

    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<ListResourcesResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return ListResourcesResult();
    }

    return ListResourcesResult();
}

ListResourceTemplatesResult ClientSession::ListResourceTemplates(const std::string& cursor) {
    auto request = std::make_shared<ListResourceTemplatesRequest>();
    request->method = "resources/templates/list";

    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<ListResourceTemplatesResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return ListResourceTemplatesResult();
    }

    return ListResourceTemplatesResult();
}

// todo: anyurl
ReadResourceResult ClientSession::ReadResource(const std::string& uri) {
    auto request = std::make_shared<ReadResourceRequest>();
    request->method = "resources/read";
    request->params.uri = uri;

    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<ReadResourceResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return ReadResourceResult();
    }

    return ReadResourceResult();
}

EmptyResult ClientSession::SubscribeResource(const std::string& uri) {
    auto request = std::make_shared<SubscribeRequest>();
    request->method = "resources/subscribe";
    request->params.uri = uri;

    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<EmptyResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return EmptyResult();
    }

    return EmptyResult();
}

EmptyResult ClientSession::UnsubscribeResource(const std::string& uri) {
    auto request = std::make_shared<UnsubscribeRequest>();
    request->method = "resources/unsubscribe";
    request->params.uri = uri;

    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<EmptyResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return EmptyResult();
    }

    return EmptyResult();
}

ListPromptsResult ClientSession::ListPrompts(const std::string& cursor) {
    auto request = std::make_shared<ListPromptsRequest>();
    request->method = "prompts/list";
    if (!cursor.empty()) {
        request->params->cursor = cursor;
    }

    auto result = SendRequest(request);

    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<ListPromptsResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return ListPromptsResult();
    }

    return ListPromptsResult();
}

GetPromptResult ClientSession::GetPrompt(const std::string& name, 
                            const std::map<std::string, std::string>& arguments) {
    auto request = std::make_shared<GetPromptRequest>();
    request->method = "prompts/get";
    request->params.name = name;
    request->params.arguments = arguments;

    auto result = SendRequest(request);
    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<GetPromptResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return GetPromptResult();
    }

    return GetPromptResult();
}

CompleteResult ClientSession::Complete(const ResourceTemplateReference& ref, 
                        const std::map<std::string, std::string>& argument,
                        const std::map<std::string, std::string>& contextArguments) {
    auto request = std::make_shared<CompleteRequest1>();
    request->method = "completion/complete";
    CompleteRequestParams1 pa;
    pa.ref = ref;
    if (argument.find("name") != argument.end()) {
        pa.argument.name = argument.at("name");
    }

    if (argument.find("value") != argument.end()) {
        pa.argument.value = argument.at("value");
    }
    pa.context->arguments = contextArguments;

    request->params = pa;

    auto result = SendRequest(request);
    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<CompleteResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return CompleteResult();
    }

    return CompleteResult();
}

CompleteResult ClientSession::Complete(const PromptReference& ref, 
                        const std::map<std::string, std::string>& argument,
                        const std::map<std::string, std::string>& contextArguments) {
    auto request = std::make_shared<CompleteRequest2>();
    request->method = "completion/complete";
    CompleteRequestParams2 pa;
    pa.ref = ref;
    if (argument.find("name") != argument.end()) {
        pa.argument.name = argument.at("name");
    }

    if (argument.find("value") != argument.end()) {
        pa.argument.value = argument.at("value");
    }
    pa.context->arguments = contextArguments;

    request->params = pa;

    auto result = SendRequest(request);
    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<CompleteResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return CompleteResult();
    }

    return CompleteResult();
}

EmptyResult ClientSession::SetLoggingLevel(const std::string& level) {
    auto request = std::make_shared<SetLevelRequest>();
    request->method = "logging/setLevel";
    request->params.level = level;

    auto result = SendRequest(request);
    try {
        auto response = result.back().data.get<JSONRPCResponse>();

        auto ret = response.result.get<EmptyResult>();
        return ret;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("ClientSession::Initialize deserialize error: {}", e.what());
        return EmptyResult();
    }

    return EmptyResult();
}

void ClientSession::SendRootsListChanged() {

}

void ClientSession::SendProgressNotification() {
    
}

void ClientSession::StoreServerResponse(std::string res) {
    std::lock_guard<std::mutex> lock(lock_);

    serverSSENotificationResponse_.append(res);
    
    // Parse the accumulated response data
    std::vector<SSEResponse> parsedResponses = ParseSSEResponse(serverSSENotificationResponse_);
    
    // Add parsed responses to the notifications vector
    for (const auto& response : parsedResponses) {
        serverSSENotifications_.push_back(response);
    }
    
    // Clear the parsed portion from the accumulated response
    // We need to find the last complete message and remove everything up to that point
    size_t lastCompleteMessageEnd = 0;
    std::istringstream stream(serverSSENotificationResponse_);
    std::string line;
    bool hasEvent = false;
    bool hasData = false;
    
    while (std::getline(stream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) {
            // End of a message
            if (hasEvent && hasData) {
                // Update the position of the last complete message
                lastCompleteMessageEnd = stream.tellg();
            }
            hasEvent = false;
            hasData = false;
        } else if (line.substr(0, 6) == "event:") {
            hasEvent = true;
        } else if (line.substr(0, 5) == "data:") {
            hasData = true;
        }
    }
    
    // Handle the last message if there's no trailing empty line
    if (hasEvent && hasData) {
        lastCompleteMessageEnd = serverSSENotificationResponse_.size();
    }
    
    // Remove the parsed portion from the accumulated response
    if (lastCompleteMessageEnd > 0) {
        serverSSENotificationResponse_ = serverSSENotificationResponse_.substr(lastCompleteMessageEnd);
        // If the remaining string starts with a newline, remove it
        if (!serverSSENotificationResponse_.empty() && serverSSENotificationResponse_[0] == '\n') {
            serverSSENotificationResponse_ = serverSSENotificationResponse_.substr(1);
        }
    }
}

void ClientSession::SendNotification(nlohmann::json notification) {
    // auto noti = std::make_shared<JSONRPCNotification>();
    // noti->params = notification;
    notification["jsonrpc"] = "2.0";
    // notification["id"] = GenerateReuqestId();

    auto response = transport_->SendMessage(notification, sessionId_);

    spdlog::info("response: {}", response.body); 
    spdlog::info("response: {}", response.body); 
}

template<typename T>
std::vector<SSEResponse> ClientSession::SendRequest(std::shared_ptr<T> request) {
    // auto rpcRequest = std::make_shared<JSONRPCRequest>();
    nlohmann::json rpcRequest = nlohmann::json(*request);
    rpcRequest["id"] = GenerateReuqestId();
    rpcRequest["method"] = request->method;
    rpcRequest["jsonrpc"] = "2.0";

    auto response = transport_->SendMessage(rpcRequest, sessionId_);

    spdlog::info("response: {}", response.body); 

    std::string this_session_id = "";
    if (response.headers.find("mcp-session-id") != response.headers.end()) {
        this_session_id = response.headers["mcp-session-id"];
    } else if (response.headers.find("Mcp-Session-Id") != response.headers.end()) {
        this_session_id = response.headers["Mcp-Session-Id"];
    }
    if (sessionId_.empty()) {
        sessionId_ = this_session_id;
    } else if (sessionId_ != this_session_id) {
        spdlog::info("ClientSession::SendRequest error session_id, expected: {}, actual: {}", sessionId_, this_session_id);
        return {};
    }

    try {
        // nlohmann::json res = nlohmann::json::parse(response.body);
        std::vector<SSEResponse> res = ParseSSEResponse(response.body);
        return res;
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("ClientSession::SendRequest json parse error: {}", e.what());
        return {};
    }
    return {};

}

std::vector<SSEResponse> ClientSession::ParseSSEResponse(std::string body) {
    std::vector<SSEResponse> responses;
    std::istringstream stream(body);
    std::string line;
    SSEResponse currentResponse;
    bool hasEvent = false;
    bool hasData = false;

    while (std::getline(stream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            // End of a message
            if (hasEvent && hasData) {
                responses.push_back(currentResponse);
            }
            hasEvent = false;
            hasData = false;
            currentResponse = SSEResponse();
        } else if (line.substr(0, 6) == "event:") {
            currentResponse.event = line.substr(6);
            // Remove leading space if present
            if (!currentResponse.event.empty() && currentResponse.event[0] == ' ') {
                currentResponse.event = currentResponse.event.substr(1);
            }
            hasEvent = true;
        } else if (line.substr(0, 5) == "data:") {
            std::string data = line.substr(5);
            // Remove leading space if present
            if (!data.empty() && data[0] == ' ') {
                data = data.substr(1);
            }
            try {
                currentResponse.data = nlohmann::json::parse(data);
                hasData = true;
            } catch (const nlohmann::json::parse_error& e) {
                spdlog::error("ClientSession::ParseSSEResponse json parse error: {}", e.what());
            }
        }
    }

    // Handle the last message if there's no trailing empty line
    if (hasEvent && hasData) {
        responses.push_back(currentResponse);
    }

    return responses;
}

void ClientSession::StartSSElisten() {
    listenThread_ = std::make_shared<std::thread>([&](){
        transport_->ListenNotification(sessionId_);
    });
}

}
}
