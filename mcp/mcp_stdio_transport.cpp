#include <memoria/mcp/mcp_stdio_transport.hpp>
#include <nlohmann/json.hpp>

namespace memoria::mcp {

using json = nlohmann::json;

MCPStdioTransport::MCPStdioTransport(MCPServer& server) : server_(server) {}

void MCPStdioTransport::run() {
    std::cerr << "Starting MCP server with stdio transport." << std::endl;
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            json request = json::parse(line);
            std::string tool_name = request["tool_name"].get<std::string>();
            json args = request.count("args") ? request["args"] : json::object();

            json result = server_.execute_tool(tool_name, args);
            json response = {{"result", result}};
            std::cout << response.dump() << std::endl;
        } catch (const json::exception& e) {
            json error_response = {{"error", "JSON parsing error: " + std::string(e.what())}};
            std::cout << error_response.dump() << std::endl;
        } catch (const std::exception& e) {
            json error_response = {{"error", "Server error: " + std::string(e.what())}};
            std::cout << error_response.dump() << std::endl;
        }
    }
}

} // namespace memoria::mcp
