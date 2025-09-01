#ifndef MCP_CLIENT_TRANSPORT_H_
#define MCP_CLIENT_TRANSPORT_H_

#include "../common/types.h"
#include "http_client.h"

namespace cosate {
namespace mcp_client {

class Transport {
public:
    virtual ~Transport() = default;

    virtual HttpRequestCompleteInfo SendMessage(nlohmann::json request, std::string sessionId) = 0;

    virtual void ListenNotification(std::string sessionId) = 0;

    virtual void SetStop(bool s) = 0;
};

}
}

#endif