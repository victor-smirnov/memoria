
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
#include <boost/regex/pending/unicode_iterator.hpp>

#include <cctype>

namespace memoria::hermes {


struct TplSpaceData {
    Optional<bool> left_space;
    Optional<bool> right_space;

    operator bool() const {
        return left_space.is_initialized() || right_space.is_initialized();
    }
};

struct TemplateConstants {
    using IteratorT = boost::u8_to_u32_iterator<U8StringView::const_iterator>;

    static constexpr U8StringView CODE_ATTR         = "code";
    static constexpr U8StringView NODE_NAME_ATTR    = "astNodeName";

    static constexpr int64_t CODE_MIN               = 100;

    static constexpr int64_t FOR_STMT_CODE          = 100;
    static constexpr U8StringView FOR_STMT_NAME     = "ForStmt";

    static constexpr int64_t IF_STMT_CODE           = 101;
    static constexpr U8StringView IF_STMT_NAME      = "IfStmt";

    static constexpr int64_t ELSE_STMT_CODE         = 102;
    static constexpr U8StringView ELSE_STMT_NAME    = "ElseStmt";

    static constexpr int64_t SET_STMT_CODE          = 103;
    static constexpr U8StringView SET_STMT_NAME     = "SetStmt";

    static constexpr int64_t VAR_STMT_CODE          = 104;
    static constexpr U8StringView VAR_STMT_NAME     = "VarStmt";

    static constexpr int64_t CODE_MAX               = 105;


    static constexpr U8StringView VARIABLE_NAME     = "variable";
    static constexpr U8StringView EXPRESSION_NAME   = "expression";
    static constexpr U8StringView STATEMENTS_NAME   = "statements";
    static constexpr U8StringView ELSE_NAME         = "else";

    static constexpr U8StringView LEFT_SPACE_NAME   = "left_space";
    static constexpr U8StringView RIGHT_SPACE_NAME  = "right_space";
    static constexpr U8StringView SPACE_DATA_NAME   = "right_space";

    static constexpr U8StringView END_SPACE_DATA_NAME   = "end_space_data";
    static constexpr U8StringView START_SPACE_DATA_NAME = "start_space_data";
    static constexpr U8StringView STRIP_SPACE_NAME  = "strip_space";


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

    static ObjectPtr new_space_data(const TplSpaceData& data)
    {
        if (data)
        {
            auto map = current_ctr()->new_map();

            if (data.left_space.is_initialized()) {
                map->put_dataobject<Boolean>(LEFT_SPACE_NAME, data.left_space.get());
            }

            if (data.right_space.is_initialized()) {
                map->put_dataobject<Boolean>(RIGHT_SPACE_NAME, data.right_space.get());
            }

            return map->as_object();
        }
        else {
            return {};
        }
    }

    static bool is_empty(U8StringView text)
    {
        boost::u8_to_u32_iterator<U8StringView::const_iterator> ii = text.begin();

        while (ii != text.end())
        {
            int ch = *ii;
            if (!std::iswspace(ch)) {
                return false;
            }

            ++ii;
        }

        return true;
    }

    static size_t find_first_line(U8StringView text)
    {
        auto pos = text.find_first_of("\n");
        if (MMA_UNLIKELY(pos == text.npos))
        {
            auto rpos = text.find_first_of("\r"); // Classic MacOS.
            if (MMA_UNLIKELY(rpos == text.npos)) {
                return 0;
            }
            else {
                return rpos + 1;
            }
        }
        else {
            return pos + 1;
        }
    }

    static size_t find_last_line(U8StringView text)
    {
        auto pos = text.find_last_of("\n");
        if (MMA_UNLIKELY(pos == text.npos))
        {
            auto rpos = text.find_last_of("\r"); // Classic MacOS.
            if (MMA_UNLIKELY(rpos == text.npos)) {
                return 0;
            }
            else {
                return rpos + 1;
            }
        }
        else {
            return pos + 1;
        }
    }

    static U8StringView strip_start_ws(U8StringView text)
    {
        boost::u8_to_u32_iterator<U8StringView::const_iterator> ii = text.begin();

        while (ii != text.end())
        {
            int ch = *ii;
            if (std::iswspace(ch)) {
                ++ii;
            }
            else {
                break;
            }
        }

        size_t pos = offset(text.begin(), ii);
        return text.substr(pos);
    }


    static U8StringView trim_end_ws(U8StringView text)
    {
        boost::u8_to_u32_iterator<U8StringView::const_iterator> ii = text.begin();

        size_t pos {};
        bool was_non_space{true};
        while (ii != text.end())
        {
            int ch = *ii;
            if (!std::iswspace(ch)) {
                was_non_space = true;
            }
            else if (was_non_space) {
                pos = offset(text.begin(), ii);
                was_non_space = false;
            }

            ++ii;
        }

        return text.substr(0, pos);
    }

    static U8StringView trim_last_line(U8StringView text)
    {
        size_t pos = find_last_line(text);
        U8StringView last_line = text.substr(pos);

        if (is_empty(last_line)) {
            return text.substr(0, pos);
        }
        else {
            return text;
        }
    }

    static U8StringView strip_first_line(U8StringView text)
    {
        size_t pos = find_first_line(text);
        U8StringView first_line = text.substr(0, pos);

        if (is_empty(first_line)) {
            return text.substr(pos);
        }
        else {
            return text;
        }
    }

    static bool is_strip_space(Optional<bool> val) {
        return val.is_initialized() && !val.get();
    }

    static bool is_preserve_line(Optional<bool> val) {
        return val.is_initialized() && val.get();
    }

    static bool is_strip_space(const ObjectPtr& mapo, U8StringView prop)
    {
        if (mapo->is_object_map())
        {
            ObjectMapPtr map = mapo->as_object_map();
            auto val = map->get(prop);
            if (val->is_not_null()) {
                return !val->to_bool();
            }
        }
        return false;
    }

    static bool is_preserve_line(const ObjectPtr& mapo, U8StringView prop)
    {
        if (mapo->is_object_map())
        {
            ObjectMapPtr map = mapo->as_object_map();
            auto val = map->get(prop);
            if (val->is_not_null()) {
                return val->to_bool();
            }
        }
        return false;
    }

    static void update_string(ObjectArrayPtr& array, size_t idx, U8StringView out, U8StringView str) {
        if (out.length() != str.length()) {
            array->set(
                idx,
                current_ctr()->new_dataobject<Varchar>(out)->as_object()
            );
        }
    }

    static ObjectPtr process_inner_space(const ObjectPtr& stmts, Optional<bool> start, Optional<bool> end)
    {
        if (stmts->is_varchar())
        {
            StringValuePtr txt = stmts->as_varchar();
            U8StringView out = *txt->view();

            if (is_strip_space(start)) {
                out = strip_start_ws(*txt->view());
            }
            else if (!is_preserve_line(start)) {
                out = strip_first_line(*txt->view());
            }

            if (is_strip_space(end)) {
                out = trim_end_ws(out);
            }
            else if (!is_preserve_line(end)) {
                out = trim_last_line(*txt->view());
            }

            if (out.length() != txt->view()->length()) {
                return current_ctr()->new_dataobject<Varchar>(out)->as_object();
            }
        }
        else if (stmts->is_object_array())
        {
            ObjectArrayPtr array = stmts->as_object_array();
            if (array->size() == 0) {
                return {};
            }
            else if (array->size() == 1) {
                auto new_str = process_inner_space(array->get(0), start, end);
                if (new_str->is_not_null()) {
                    array->set(0, new_str);
                }
            }
            else
            {
                ObjectPtr blk_start = array->get(0);
                if (blk_start->is_varchar())
                {
                    StringValuePtr str = blk_start->as_varchar();
                    if (is_strip_space(start))
                    {
                        U8StringView out = strip_start_ws(*str->view());
                        update_string(array, 0, out, *str->view());
                    }
                    else if (!is_preserve_line(start))
                    {
                        U8StringView out = strip_first_line(*str->view());
                        update_string(array, 0, out, *str->view());
                    }
                }

                ObjectPtr blk_end = array->get(array->size() - 1);
                if (blk_end->is_varchar())
                {
                    StringValuePtr str = blk_end->as_varchar();

                    if (is_strip_space(end))
                    {
                        U8StringView out = trim_end_ws(*str->view());
                        update_string(array, array->size() - 1, out, *str->view());
                    }
                    else if (!is_preserve_line(end))
                    {
                        U8StringView out = trim_last_line(*str->view());
                        update_string(array, array->size() - 1, out, *str->view());
                    }
                }
            }
        }

        return {};
    }

    static void process_outer_space(ObjectPtr blocks)
    {
        if (blocks->is_object_array())
        {
            ObjectArrayPtr array = blocks->as_object_array();
            for (size_t c = 1; c < array->size(); c++)
            {
                ObjectPtr item = array->get(c);
                if (item->is_object_map())
                {
                    ObjectPtr prev = array->get(c - 1);
                    ObjectPtr next = ((c + 1) < array->size()) ? array->get(c + 1) : ObjectPtr{};
                    if (prev->is_varchar() || next->is_varchar())
                    {
                        ObjectMapPtr map = item->as_object_map();
                        ObjectPtr code = map->get(CODE_ATTR);
                        if (code->is_not_null())
                        {
                            int64_t code = map->get(CODE_ATTR)->to_i64();
                            auto do_strip_space = map->get(STRIP_SPACE_NAME);
                            if (code >= CODE_MIN && code <= CODE_MAX && do_strip_space->is_not_null())
                            {
                                ObjectPtr start_sps = map->get(START_SPACE_DATA_NAME);
                                if (prev->is_varchar())
                                {
                                    auto str = prev->as_varchar();
                                    if (is_strip_space(start_sps, LEFT_SPACE_NAME))
                                    {
                                        U8StringView out = trim_end_ws(*str->view());
                                        update_string(array, c - 1, out, *str->view());
                                    }
                                    else if (!is_preserve_line(start_sps, LEFT_SPACE_NAME))
                                    {
                                        U8StringView out = trim_last_line(*str->view());
                                        update_string(array, c - 1, out, *str->view());
                                    }
                                }

                                ObjectPtr end_sps = map->get(END_SPACE_DATA_NAME);
                                if (next->is_varchar())
                                {
                                    auto str = next->as_varchar();
                                    if (is_strip_space(end_sps, RIGHT_SPACE_NAME)) {
                                        U8StringView out = strip_start_ws(*str->view());
                                        update_string(array, c + 1, out, *str->view());
                                    }
                                    else if (!is_preserve_line(end_sps, RIGHT_SPACE_NAME))
                                    {
                                        U8StringView out = strip_first_line(*str->view());
                                        update_string(array, c + 1, out, *str->view());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    static size_t offset(const U8StringView::const_iterator& start, const IteratorT& current) {
        return std::distance(start, current.base());
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

        map->put_dataobject<Varchar>(VARIABLE_NAME, variable.identifier);
        map->put(EXPRESSION_NAME, expression);

        map->put(STATEMENTS_NAME, blocks);
        map->put(END_SPACE_DATA_NAME, new_space_data(end_space_data));

        TplSpaceData start_space_data{left_open_space, right_open_space};
        map->put(START_SPACE_DATA_NAME, new_space_data(start_space_data));

        map->put_dataobject<Boolean>(STRIP_SPACE_NAME, true);

        auto res = process_inner_space(blocks, right_open_space, end_space_data.left_space);
        if (res->is_not_null()) {
            map->put(STATEMENTS_NAME, blocks);
        }

        process_outer_space(blocks);

        return map->as_object();
    }
};


struct TplSetStatement: TemplateConstants {
    Optional<bool> left_space;
    path::ast::IdentifierNode variable;
    ObjectPtr expression;
    Optional<bool> right_space;

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(SET_STMT_CODE, SET_STMT_NAME);

        map->put_dataobject<Varchar>(VARIABLE_NAME, variable.identifier);
        map->put(EXPRESSION_NAME, expression);

        TplSpaceData space_data{left_space, right_space};
        map->put(START_SPACE_DATA_NAME, new_space_data(space_data));

        map->put_dataobject<Boolean>(STRIP_SPACE_NAME, true);

        return map->as_object();
    }
};

struct TplVarStatement: TemplateConstants {
    ObjectPtr expression;

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(VAR_STMT_CODE, VAR_STMT_NAME);
        map->put(EXPRESSION_NAME, expression);
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

        map->put_dataobject<Boolean>(STRIP_SPACE_NAME, true);

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
        map->put_dataobject<Boolean>(STRIP_SPACE_NAME, true);

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
    memoria::hermes::TplSetStatement,
    (memoria::Optional<bool>, left_space)
    (memoria::hermes::path::ast::IdentifierNode, variable)
    (memoria::hermes::ObjectPtr, expression)
    (memoria::Optional<bool>, right_space)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplVarStatement,
    (memoria::hermes::ObjectPtr, expression)
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
