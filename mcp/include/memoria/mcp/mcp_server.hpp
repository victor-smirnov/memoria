#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace memoria::mcp {

using json = nlohmann::json;

// Define a type for the tool function: takes JSON args, returns JSON result
using ToolFunction = std::function<json(const json&)>;

struct ToolInfo {
    std::string description;
    json parameters_schema;
    ToolFunction function;
};

class MCPServer {
public:
    MCPServer();

    // Register a tool with its description, parameter schema, and implementation
    void register_tool(
        const std::string& name,
        const std::string& description,
        const json& parameters_schema,
        ToolFunction function
    );

    // Get the full schema of all registered tools
    json get_tools_schema() const;

    // Execute a registered tool
    json execute_tool(const std::string& tool_name, const json& args);

private:
    std::unordered_map<std::string, ToolInfo> tools_;
};

} // namespace memoria::mcp
