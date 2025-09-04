#ifndef MCP_CLIENT_SESSION_H_
#define MCP_CLIENT_SESSION_H_

#include "../common/types.h"

namespace mcp {
namespace client {

struct SSEResponse {
    std::string event;
    nlohmann::json data;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SSEResponse, event, data)
};

class Session {
public:
    virtual ~Session() = default;

    template<typename T>
    std::shared_ptr<JSONRPCRequest> MakeJsonRPC2(std::shared_ptr<T> request) {
        auto rpcRequest = std::make_shared<JSONRPCRequest>();
        rpcRequest->params = (*request);
        return rpcRequest;
    }

};

}
}

#endif
