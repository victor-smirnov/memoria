
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

#include <memoria/core/hermes/hermes.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

#include "hermes_ctr_builder.hpp"
#include "path/ast/expressionnode.h"
#include "path/ast/identifiernode.h"

#include "path/parser/hermes_path_ast_converter.h"

#include <boost/hana.hpp>

namespace memoria::hermes {


struct TplSpaceData {
    Optional<bool> left_space;
    Optional<bool> right_space;
};

struct TemplateConstants {
    static constexpr U8StringView CODE_ATTR         = "code";
    static constexpr U8StringView NODE_NAME_ATTR    = "astNodeName";

    static constexpr int64_t FOR_STMT_CODE          = 100;
    static constexpr U8StringView FOR_STMT_NAME     = "ForStmt";

    static constexpr int64_t IF_STMT_CODE           = 101;
    static constexpr U8StringView IF_STMT_NAME      = "IfStmt";

    static constexpr int64_t ELSE_STMT_CODE         = 102;
    static constexpr U8StringView ELSE_STMT_NAME    = "ElseStmt";

    static constexpr U8StringView VARIABLE_NAME     = "variable";
    static constexpr U8StringView EXPRESSION_NAME   = "expression";
    static constexpr U8StringView STATEMENTS_NAME   = "statements";
    static constexpr U8StringView ELSE_NAME         = "else";

    static constexpr U8StringView LEFT_SPACE_NAME   = "left_space";
    static constexpr U8StringView RIGHT_SPACE_NAME  = "right_space";
    static constexpr U8StringView SPACE_DATA_NAME   = "right_space";

    static constexpr U8StringView END_SPACE_DATA_NAME   = "end_space_data";
    static constexpr U8StringView START_SPACE_DATA_NAME = "start_space_data";


    static ObjectMapPtr new_ast_node(int64_t code, const U8StringView& name) {
        auto map = current_ctr()->new_map();
        map->put_dataobject<BigInt>(CODE_ATTR, code);
        map->put_dataobject<Varchar>(NODE_NAME_ATTR, name);
        return map;
    }

    static ObjectArrayPtr new_object_array(const std::vector<path::ast::HermesValueNode>& array)
    {
        auto harray = current_ctr()->new_array();
        for (auto& item: array) {
            harray->append(std::move(item.value));
        }
        return harray;
    }

    static ObjectPtr new_space_data(const TplSpaceData& data) {
        auto map = current_ctr()->new_map();

        if (data.left_space.is_initialized()) {
            map->put_dataobject<Boolean>(LEFT_SPACE_NAME, data.left_space.get());
        }

        if (data.right_space.is_initialized()) {
            map->put_dataobject<Boolean>(RIGHT_SPACE_NAME, data.right_space.get());
        }

        return map->as_object();
    }
};



struct TplForStatement: TemplateConstants {
    Optional<bool> left_open_space;
    path::ast::IdentifierNode variable;
    ObjectPtr expression;
    Optional<bool> right_open_space;
    ObjectPtr blocks;
    TplSpaceData end_space_data;

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(FOR_STMT_CODE, FOR_STMT_NAME);

        auto identifier = path::parser::HermesASTConverter::new_identifier(*current_ctr(), variable, false);

        map->put(VARIABLE_NAME, identifier->as_object());
        map->put(EXPRESSION_NAME, expression);

        map->put(STATEMENTS_NAME, blocks);
        map->put(END_SPACE_DATA_NAME, new_space_data(end_space_data));
        map->put(START_SPACE_DATA_NAME, new_space_data(TplSpaceData{left_open_space, right_open_space}));

        return map->as_object();
    }
};

struct TplIfStatement;
struct TplElseStatement;


using TplIfAltBranch = boost::variant<
    std::vector<ObjectPtr>,
    path::ast::HermesValueNode,
    TplSpaceData,
    boost::recursive_wrapper<TplIfStatement>,
    boost::recursive_wrapper<TplElseStatement>
>;

struct TplElseStatement: TemplateConstants {
    Optional<bool> left_open_space;
    ObjectPtr blocks;
    Optional<bool> right_open_space;
    TplSpaceData end_space_data;

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(ELSE_STMT_CODE, ELSE_STMT_NAME);
        map->put(STATEMENTS_NAME, blocks);

        map->put(START_SPACE_DATA_NAME, new_space_data(TplSpaceData{left_open_space, right_open_space}));
        map->put(END_SPACE_DATA_NAME, new_space_data(end_space_data));

        return map->as_object();
    }
};



struct TplIfStatement: TemplateConstants {
    Optional<bool> left_open_space;
    ObjectPtr expression;
    Optional<bool> right_open_space;
    ObjectPtr blocks;

    TplIfAltBranch alt_branch;



    struct AltOp {
        ObjectPtr value;
        U8StringView prop{ELSE_NAME};
    };

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(IF_STMT_CODE, IF_STMT_NAME);
        map->put(EXPRESSION_NAME, expression);
        map->put(STATEMENTS_NAME, blocks);

        auto visitor = boost::hana::overload(
            [&](const std::vector<ObjectPtr>& stts) -> AltOp {
                return {current_ctr()->new_array(stts)->as_object()};
            },
            [](const path::ast::HermesValueNode& node) -> AltOp {
                return {std::move(node.value)};
            },
            [](const TplIfStatement& node) -> AltOp {
                return {std::move(((path::ast::HermesValueNode)node).value)};
            },
            [](const TplElseStatement& node) -> AltOp {
                return {std::move(((path::ast::HermesValueNode)node).value)};
            },
            [](const TplSpaceData& space_data) -> AltOp {
                return {new_space_data(space_data), END_SPACE_DATA_NAME};
            }
        );
        auto alt_op = boost::apply_visitor(visitor, alt_branch);
        map->put(alt_op.prop, alt_op.value);

        map->put(START_SPACE_DATA_NAME, new_space_data(TplSpaceData{left_open_space, right_open_space}));

        return map->as_object();
    }
};



}



BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplForStatement,
    (memoria::Optional<bool>, left_open_space)
    (memoria::hermes::path::ast::IdentifierNode, variable)
    (memoria::hermes::ObjectPtr, expression)
    (memoria::Optional<bool>, right_open_space)
    (memoria::hermes::ObjectPtr, blocks)
    (memoria::hermes::TplSpaceData, end_space_data)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplIfStatement,
    (memoria::Optional<bool>, left_open_space)
    (memoria::hermes::ObjectPtr, expression)
    (memoria::Optional<bool>, right_open_space)
    (memoria::hermes::ObjectPtr, blocks)
    (memoria::hermes::TplIfAltBranch, alt_branch)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplElseStatement,
    (memoria::Optional<bool>, left_open_space)
    (memoria::Optional<bool>, right_open_space)
    (memoria::hermes::ObjectPtr, blocks)
    (memoria::hermes::TplSpaceData, end_space_data)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplSpaceData,
    (memoria::Optional<bool>, left_space)
    (memoria::Optional<bool>, right_space)
)
