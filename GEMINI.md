# User-provided instructions

Our current goal is to build a simple MCP server that will be implementing agentic development workflow for the project. Eventually this MCP server will be pretty complex in terms of both the number of available tools and complexity of functionality. But for now we just need to bootstrap it. Here goes a minimalist knowledge base necessary for bootstrapping the server.

The top-level project Memoria is a full stack of subsystems sufficient for building a converged database engine. It contains data modeling and serialization format (Hermes), networking subsystem (HRPC), collection library (Containers), storage subsystem (Storage) and programming language and data processing subsystem (DSLEngine). The project is entirely written in modern C++ (C++17/20) and is using metaprogramming techniques intensively. Theses is a dedicated tool (Memoria Build Tool or mbt) for project-specific code analysis and generation.

Project build process is provided by CMake scripts and performed with ninja build tool. CMake scripts are ideomatic and modular. Dependencies are provided by VCPKG and custom repository (currently for Seastar framework that is missing in main VCPKG repository). Build scripts are highly modular, with strict minimum-dependency policy between modules.

MCP server resides in the mcp subfolder of top-level folder. This folder also contains include directories (mcp/include) and .cpp source files. 

For now our goal is to write a simple minimalist MCP server, continuing the work in the mcp folder. The server should use two transports (stdio and HTTP) enables by command line switch (Boost Program Options). Let's for now this server is exporting one simple tool adding two numbers. 

**Important!** Please don't build the project yourself. The build process requires some external setup (configuring VCPKG), so the User will be running it manually.

**Important!** All dependencies are managed by the User. You can safely assume that required libraries are already linked with and corresponding headers are included. Just mention what you are going to use and the User will add those libraries if necessary.

# Agent's memory

In the last session, I refactored the MCP server to separate core logic from transport mechanisms. I introduced `MCPServer` for tool management, `MCPStdioTransport` for stdio communication, and `MCPHttpTransport` for HTTP communication. I also fixed a double-nesting issue in JSON responses for HTTP and ensured consistent JSON output for stdio transport. The `add_numbers` tool was successfully tested in both HTTP and stdio modes.