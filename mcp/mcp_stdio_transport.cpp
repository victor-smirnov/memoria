// Copyright 2025 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-20.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
