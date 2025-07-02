#pragma once

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
