#pragma once

#include <memoria/mcp/mcp_server.hpp>
#include <iostream>

namespace memoria::mcp {

class MCPStdioTransport {
public:
    MCPStdioTransport(MCPServer& server);
    void run();

private:
    MCPServer& server_;
};

} // namespace memoria::mcp
