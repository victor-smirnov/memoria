
// Copyright 2022 Victor Smirnov
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

#pragma once

#include <memoria/core/hermes/container.hpp>

#include <ostream>

namespace memoria {
namespace hermes {

struct TplASTCodes {
    static constexpr NamedCode CODE         = NamedCode(0, "Code");
    static constexpr NamedCode FOR_STMT     = NamedCode(1, "ForStmt");
    static constexpr NamedCode IF_STMT      = NamedCode(2, "IfStmt");
    static constexpr NamedCode ELSE_STMT    = NamedCode(3, "ElseStmt");
    static constexpr NamedCode SET_STMT     = NamedCode(4, "SetStmt");
    static constexpr NamedCode VAR_STMT     = NamedCode(5, "VarStmt");

    static constexpr NamedCode VARIABLE     = NamedCode(6, "variable");
    static constexpr NamedCode EXPRESSION   = NamedCode(7, "expression");
    static constexpr NamedCode STATEMENTS   = NamedCode(8, "statements");
    static constexpr NamedCode ELSE         = NamedCode(9, "else");

    //static constexpr NamedCode STRIP_SPACE      = NamedCode(15, "strip_space");
    static constexpr NamedCode NODE_NAME_ATTR   = NamedCode(16, "astNodeName");

    static constexpr NamedCode TOP_OUTER_SPACE = NamedCode(20, "top_outer_space");
    static constexpr NamedCode TOP_INNER_SPACE = NamedCode(21, "top_inner_space");
    static constexpr NamedCode BOTTOM_INNER_SPACE = NamedCode(22, "bottom_inner_space");
    static constexpr NamedCode BOTTOM_OUTER_SPACE = NamedCode(23, "bottom_outer_space");
};


PoolSharedPtr<HermesCtr> parse_template(U8StringView text, bool node_names = false);

void render(const ObjectPtr& tpl, const ObjectPtr& data, std::ostream& out);
void render(U8StringView tpl, const ObjectPtr& data, std::ostream& out);

}}
