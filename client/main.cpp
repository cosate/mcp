#include <iostream>
#include "client_session.h"
#include "http_client.h"
#include "streamable_http_transport.h"

ienusing namespace mcp::client;

int main() {
    auto httpClient = std::make_shared<HttpClient>();
    auto transport = std::make_shared<StreamableHttpTransport>("http://example.com", httpClient);

    auto session = std::make_shared<ClientSession>(transport);
    return 0;
}
