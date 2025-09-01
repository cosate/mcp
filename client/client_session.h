#ifndef MCP_CLIENT_CLIENT_SESSION_H_
#define MCP_CLIENT_CLIENT_SESSION_H_

#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include "transport.h"
#include "../common/types.h"
#include "session.h"


namespace cosate {
namespace mcp_client {

class ClientSession : public Session {
public:
    explicit ClientSession(std::shared_ptr<Transport> transport, Implementation clientInfo = Implementation());
    
    virtual ~ClientSession();

    void SetClientCapabilities();

public:
    
    InitializeResult Initialize();

    EmptyResult SendPing();
    
    ListToolsResult ListTools(const std::string& cursor = "");

    CallToolResult CallTool(const std::string& name, 
                            const std::map<std::string, std::string>& arguments = {});
    
    ListResourcesResult ListResources(const std::string& cursor = "");

    ListResourceTemplatesResult ListResourceTemplates(const std::string& cursor = "");

    // todo: anyurl
    ReadResourceResult ReadResource(const std::string& uri);

    // todo: anyurl
    EmptyResult SubscribeResource(const std::string& uri);

    // todo: anyurl
    EmptyResult UnsubscribeResource(const std::string& uri);
    
    ListPromptsResult ListPrompts(const std::string& cursor = "");

    GetPromptResult GetPrompt(const std::string& name, 
                              const std::map<std::string, std::string>& arguments = {});
    
    CompleteResult Complete(const ResourceTemplateReference& ref, 
                           const std::map<std::string, std::string>& argument,
                           const std::map<std::string, std::string>& contextArguments = {});
    
    CompleteResult Complete(const PromptReference& ref, 
                           const std::map<std::string, std::string>& argument,
                           const std::map<std::string, std::string>& contextArguments = {});
    
    EmptyResult SetLoggingLevel(const std::string& level);

    void SendRootsListChanged();
    
    void SendProgressNotification();

    template<typename T>
    std::vector<SSEResponse> SendRequest(std::shared_ptr<T> request);

    void SendNotification(nlohmann::json notification);

    void StoreServerResponse(std::string res);

protected:
    void StartSSElisten();

    std::vector<SSEResponse> ParseSSEResponse(std::string);

private:
    std::string GenerateReuqestId();

    std::atomic<uint64_t> request_id_counter_;
    
    std::shared_ptr<Transport> transport_;

    Implementation clientInfo_;

    ClientCapabilities clientCapabilities_;

    ServerCapabilities serverCapabilities_;

    std::mutex lock_;

    std::string sessionId_;

    std::atomic<bool> stopped_ = true;

    std::shared_ptr<std::thread> listenThread_;

    std::vector<SSEResponse> serverSSENotifications_;

    std::string serverSSENotificationResponse_;

};

}
}

#endif