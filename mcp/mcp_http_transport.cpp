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
