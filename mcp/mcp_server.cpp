#include <memoria/mcp/mcp_server.hpp>
#include <stdexcept>

namespace memoria::mcp {

MCPServer::MCPServer() {
    // Register the 'init' tool internally
    register_tool(
        "init",
        "Provides the schema of all available tools",
        json::parse("{\"type\":\"object\",\"properties\":{},\"required\":[]}"),
        [this](const json& args) {
            return get_tools_schema();
        }
    );
}

void MCPServer::register_tool(
    const std::string& name,
    const std::string& description,
    const json& parameters_schema,
    ToolFunction function
) {
    tools_[name] = {description, parameters_schema, function};
}

json MCPServer::get_tools_schema() const {
    json tools_schema;
    for (const auto& pair : tools_) {
        if (pair.first != "init") { // 'init' tool is internal and doesn't need to be exposed in the schema for other tools
            tools_schema[pair.first] = {
                {"description", pair.second.description},
                {"parameters", pair.second.parameters_schema}
            };
        }
    }
    return tools_schema;
}

json MCPServer::execute_tool(const std::string& tool_name, const json& args) {
    auto it = tools_.find(tool_name);
    if (it == tools_.end()) {
        throw std::runtime_error("Tool not found: " + tool_name);
    }
    return it->second.function(args);
}

} // namespace memoria::mcp
