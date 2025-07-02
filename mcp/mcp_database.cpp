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

#include <memoria/mcp/mcp_database.hpp>
#include <memoria/mcp/mcp_cmake_vtab.hpp>
#include <stdexcept>

namespace memoria::mcp {

void MCPDatabase::register_cmake_vtab()
{
    if (memoria::mcp::register_cmake_vtab(db_)) {
        throw std::runtime_error(sqlite3_errmsg(db_));
    }

    execute_query("CREATE VIRTUAL TABLE cmake_compile_commands USING cmake_commands;");
}

MCPDatabase::MCPDatabase() {
    if (sqlite3_open(":memory:", &db_)) {
        throw std::runtime_error(sqlite3_errmsg(db_));
    }
    register_cmake_vtab();
}

MCPDatabase::~MCPDatabase() {
    sqlite3_close(db_);
}

nlohmann::json MCPDatabase::execute_query(const std::string& sql) {
    nlohmann::json result = nlohmann::json::array();
    char* err_msg = nullptr;

    int rc = sqlite3_exec(db_, sql.c_str(), [](void* data, int argc, char** argv, char** azColName) {
        nlohmann::json* rows = static_cast<nlohmann::json*>(data);
        nlohmann::json row;
        for (int i = 0; i < argc; i++) {
            row[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
        rows->push_back(row);
        return 0;
    }, &result, &err_msg);

    if (rc != SQLITE_OK) {
        std::string err_str = err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error(err_str);
    }

    return result;
}


}