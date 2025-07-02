// Copyright 2025 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memoria/mcp/mcp_http_transport.hpp>
#include <nlohmann/json.hpp>

namespace memoria::mcp {

using json = nlohmann::json;

MCPHttpTransport::MCPHttpTransport(MCPServer& server, int port) : server_(server), port_(port) {}

void MCPHttpTransport::run() {
    std::cerr << "Starting MCP server with HTTP transport on port " << port_ << "." << std::endl;
    crow::SimpleApp app;

    CROW_ROUTE(app, "/init")
    ([this](){
        json response = {{"tools", server_.get_tools_schema()}};
        return crow::response(response.dump());
    });

    CROW_ROUTE(app, "/<string>").methods("POST"_method)
    ([this](const crow::request& req, std::string tool_name){
        try {
            json request_body = json::parse(req.body);
            json result = server_.execute_tool(tool_name, request_body);
            json response = {{"result", result}};
            return crow::response(response.dump());
        } catch (const json::exception& e) {
            json error_response = {{"error", "JSON parsing error: " + std::string(e.what())}};
            return crow::response(400, error_response.dump());
        } catch (const std::exception& e) {
            json error_response = {{"error", "Server error: " + std::string(e.what())}};
            return crow::response(500, error_response.dump());
        }
    });

    app.port(port_).run();
}

} // namespace memoria::mcp
