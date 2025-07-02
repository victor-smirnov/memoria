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

#include <memoria/mcp/mcp_cmake_vtab.hpp>
#include <memoria/mcp/mcp_utils.hpp>

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace memoria::mcp {

using json = nlohmann::json;

struct CMakeVTab final: sqlite3_vtab {
    json db;
};

struct CMakeVTabCursor final: sqlite3_vtab_cursor {
    size_t row_id{};
};

static int cmake_vtab_create(
    sqlite3* db,
    void* p_aux,
    int argc,
    const char* const* argv,
    sqlite3_vtab** pp_vtab,
    char** pz_err
)
{
    auto* vtab = new CMakeVTab();
    *pp_vtab = vtab;

    sqlite3_declare_vtab(db, "CREATE TABLE cmake_compile_commands(directory TEXT, command TEXT, file TEXT);");

    auto path = get_executable_path();
    path = path.parent_path().parent_path() / "compile_commands.json";

    std::ifstream ifs(path.string());
    if (!ifs.is_open()) {
        *pz_err = sqlite3_mprintf("Failed to open %s", path.c_str());
        return SQLITE_ERROR;
    }

    try {
        vtab->db = json::parse(ifs);
    } catch (const std::exception& e) {
        *pz_err = sqlite3_mprintf("Failed to parse compile_commands.json: %s", e.what());
        return SQLITE_ERROR;
    }

    return SQLITE_OK;
}

static int cmake_vtab_connect(
    sqlite3* db,
    void* p_aux,
    int argc,
    const char* const* argv,
    sqlite3_vtab** pp_vtab,
    char** pz_err
)
{
    return cmake_vtab_create(db, p_aux, argc, argv, pp_vtab, pz_err);
}

static int cmake_vtab_disconnect(sqlite3_vtab* p_vtab)
{
    delete static_cast<CMakeVTab*>(p_vtab);
    return SQLITE_OK;
}

static int cmake_vtab_destroy(sqlite3_vtab* p_vtab)
{
    return cmake_vtab_disconnect(p_vtab);
}


static int cmake_vtab_open(sqlite3_vtab* p_vtab, sqlite3_vtab_cursor** pp_cursor)
{
    auto* cursor = new CMakeVTabCursor();
    *pp_cursor = cursor;
    cursor->pVtab = p_vtab;
    return SQLITE_OK;
}

static int cmake_vtab_close(sqlite3_vtab_cursor* p_cursor)
{
    delete static_cast<CMakeVTabCursor*>(p_cursor);
    return SQLITE_OK;
}

static int cmake_vtab_filter(sqlite3_vtab_cursor* p_cursor, int idxNum, const char *idxStr, int argc, sqlite3_value **argv)
{
    static_cast<CMakeVTabCursor*>(p_cursor)->row_id = 0;
    return SQLITE_OK;
}

static int cmake_vtab_next(sqlite3_vtab_cursor* p_cursor)
{
    static_cast<CMakeVTabCursor*>(p_cursor)->row_id++;
    return SQLITE_OK;
}

static int cmake_vtab_eof(sqlite3_vtab_cursor* p_cursor)
{
    auto* vtab = static_cast<CMakeVTab*>(p_cursor->pVtab);
    return static_cast<CMakeVTabCursor*>(p_cursor)->row_id >= vtab->db.size();
}

static int cmake_vtab_column(sqlite3_vtab_cursor* p_cursor, sqlite3_context* ctx, int n)
{
    auto* vtab = static_cast<CMakeVTab*>(p_cursor->pVtab);
    auto* cursor = static_cast<CMakeVTabCursor*>(p_cursor);
    const auto& row = vtab->db[cursor->row_id];

    switch (n) {
        case 0: {
            const auto& value = row.value("directory", "");
            sqlite3_result_text(ctx, value.c_str(), -1, SQLITE_TRANSIENT);
            break;
        }
        case 1: {
            const auto& value = row.value("command", "");
            sqlite3_result_text(ctx, value.c_str(), -1, SQLITE_TRANSIENT);
            break;
        }
        case 2: {
            const auto& value = row.value("file", "");
            sqlite3_result_text(ctx, value.c_str(), -1, SQLITE_TRANSIENT);
            break;
        }
    }

    return SQLITE_OK;
}

static int cmake_vtab_rowid(sqlite3_vtab_cursor* p_cursor, sqlite3_int64* p_rowid)
{
    *p_rowid = static_cast<CMakeVTabCursor*>(p_cursor)->row_id;
    return SQLITE_OK;
}


static int cmake_vtab_best_index(sqlite3_vtab* tab, sqlite3_index_info* p_info)
{
    p_info->estimatedCost = (double)10000;
    p_info->estimatedRows = 10000;
    return SQLITE_OK;
}


static sqlite3_module cmake_vtab_module = {
    0,
    cmake_vtab_create,
    cmake_vtab_connect,
    cmake_vtab_best_index,
    cmake_vtab_disconnect,
    cmake_vtab_destroy,
    cmake_vtab_open,
    cmake_vtab_close,
    cmake_vtab_filter, // xFilter
    cmake_vtab_next,
    cmake_vtab_eof,
    cmake_vtab_column,
    cmake_vtab_rowid,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};


int register_cmake_vtab(sqlite3* db)
{
    return sqlite3_create_module(db, "cmake_commands", &cmake_vtab_module, nullptr);
}

}
