#pragma once

#include <memoria/mcp/mcp_server.hpp>
#include <crow.h>

namespace memoria::mcp {

class MCPHttpTransport {
public:
    MCPHttpTransport(MCPServer& server, int port);
    void run();

private:
    MCPServer& server_;
    int port_;
};

} // namespace memoria::mcp
