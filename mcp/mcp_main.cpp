
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

#include <memoria/tools/mcp_all.hpp>
#include <memoria/mcp/mcp_server.hpp>
#include <memoria/mcp/mcp_stdio_transport.hpp>
#include <memoria/mcp/mcp_http_transport.hpp>

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

namespace po = boost::program_options;
using json = nlohmann::json;

// This is a placeholder for the Memoria's experimental MCP server.

int main(int argc, char** argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("transport", po::value<std::string>()->default_value("stdio"), "transport type (stdio or http)")
        ("port", po::value<int>()->default_value(18080), "port for http transport");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    std::string transport = vm["transport"].as<std::string>();
    int port = vm["port"].as<int>();

    memoria::mcp::MCPServer server;

    // Register add_numbers tool
    server.register_tool(
        "add_numbers",
        "Adds two numbers",
        json::parse(R"({"type":"object","properties":{"a":{"type":"integer"},"b":{"type":"integer"}},"required":["a","b"]})"),
        [](const json& args) {
            int a = args["a"].get<int>();
            int b = args["b"].get<int>();
            return json(a + b);
        }
    );

    if (transport == "stdio") {
        memoria::mcp::MCPStdioTransport stdio_transport(server);
        stdio_transport.run();
    } else if (transport == "http") {
        memoria::mcp::MCPHttpTransport http_transport(server, port);
        http_transport.run();
    } else {
        std::cerr << "Invalid transport type: " << transport << std::endl;
        return 1;
    }

    return 0;
}


