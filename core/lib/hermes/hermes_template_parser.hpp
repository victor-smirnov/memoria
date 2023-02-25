
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

struct TemplateConstants: public TplASTCodes {
    using IteratorT = boost::u8_to_u32_iterator<U8StringView::const_iterator>;

    using ASTNodePtr = TinyObjectMap;

    static ASTNodePtr new_ast_node(const NamedCode& code) {
        auto map = current_ctr().make_tiny_map(8);
        map.put_t<Integer>(CODE, code.code());
        map.put(NODE_NAME_ATTR, code.name());
        return map;
    }

    static ObjectArray new_object_array(const std::vector<path::ast::HermesValueNode>& array)
    {
        auto harray = current_ctr().make_object_array(array.size());
        for (auto& item: array) {
            harray.push_back(std::move(item.value));
        }
        return harray;
    }

    static bool is_empty(U8StringView text)
    {
        boost::u8_to_u32_iterator<U8StringView::const_iterator> ii = text.begin();
        boost::u8_to_u32_iterator<U8StringView::const_iterator> end = text.end();

        while (ii != end)
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
        boost::u8_to_u32_iterator<U8StringView::const_iterator> end = text.end();

        while (ii != end)
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
        boost::u8_to_u32_iterator<U8StringView::const_iterator> end = text.end();

        size_t pos {};
        bool was_non_space{true};
        while (ii != end)
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

    static bool is_strip_space(const TinyObjectMap& map, const NamedCode& prop)
    {
        auto val = map.get(prop);
        if (val.is_not_null()) {
            return !val.to_bool();
        }
        return false;
    }


    static bool is_preserve_line(const TinyObjectMap& map, const NamedCode& prop)
    {
        auto val = map.get(prop);
        if (val.is_not_null()) {
            return val.to_bool();
        }
        return false;
    }

    static void update_string(ObjectArray& array, size_t idx, U8StringView out, U8StringView str) {
        if (out.length() != str.length()) {
            array.set(
                idx,
                current_ctr().make_t<Varchar>(out).as_object()
            );
        }
    }

    static Object process_inner_space(const Object& stmts, Optional<bool> top, Optional<bool> bottom)
    {
        if (stmts.is_varchar())
        {
            auto txt = stmts.as_varchar();
            U8StringView out = txt;

            if (is_strip_space(top)) {
                out = strip_start_ws(txt);
            }
            else if (!is_preserve_line(top)) {
                out = strip_first_line(txt);
            }

            if (is_strip_space(bottom)) {
                out = trim_end_ws(out);
            }
            else if (!is_preserve_line(bottom)) {
                out = trim_last_line(txt);
            }

            if (out.length() != txt.length()) {
                return current_ctr().make_t<Varchar>(out).as_object();
            }
        }
        else if (stmts.is_object_array())
        {
            ObjectArray array = stmts.as_object_array();
            if (array.size() == 0) {
                return {};
            }
            else if (array.size() == 1) {
                auto new_str = process_inner_space(array.get(0), top, bottom);
                if (new_str.is_not_null()) {
                    array.set(0, new_str);
                }
            }
            else
            {
                Object blk_start = array.get(0);
                if (blk_start.is_varchar())
                {
                    auto str = blk_start.as_varchar();
                    if (is_strip_space(top))
                    {
                        U8StringView out = strip_start_ws(str);
                        update_string(array, 0, out, str);
                    }
                    else if (!is_preserve_line(top))
                    {
                        U8StringView out = strip_first_line(str);
                        update_string(array, 0, out, str);
                    }
                }

                Object blk_end = array.get(array.size() - 1);
                if (blk_end.is_varchar())
                {
                    auto str = blk_end.as_varchar();

                    if (is_strip_space(bottom))
                    {
                        U8StringView out = trim_end_ws(str);
                        update_string(array, array.size() - 1, out, str);
                    }
                    else if (!is_preserve_line(bottom))
                    {
                        U8StringView out = trim_last_line(str);
                        update_string(array, array.size() - 1, out, str);
                    }
                }
            }
        }

        return {};
    }



    static void process_outer_space(Object blocks)
    {
        if (blocks.is_object_array())
        {
            ObjectArray array = blocks.as_object_array();
            for (size_t c = 1; c < array.size(); c++)
            {
                Object item = array.get(c);
                if (item.is_tiny_object_map())
                {
                    Object prev = array.get(c - 1);
                    Object next = ((c + 1) < array.size()) ? array.get(c + 1) : Object{};
                    if (prev.is_varchar() || next.is_varchar())
                    {
                        TinyObjectMap map = item.as_tiny_object_map();
                        Object code = map.get(CODE);
                        if (code.is_not_null())
                        {
                            int32_t icode = code.to_i32();
                            if (is_strip_space(icode))
                            {
                                if (prev.is_varchar())
                                {
                                    auto str = prev.as_varchar();
                                    if (is_strip_space(map, TOP_OUTER_SPACE))
                                    {
                                        U8StringView out = trim_end_ws(str);
                                        update_string(array, c - 1, out, str);
                                    }
                                    else if (!is_preserve_line(map, TOP_OUTER_SPACE))
                                    {
                                        U8StringView out = trim_last_line(str);
                                        update_string(array, c - 1, out, str);
                                    }
                                }

                                if (next.is_varchar())
                                {
                                    auto str = next.as_varchar();

                                    Optional<bool> b_o_s = get_bottom_outer_space(map);

                                    if (is_strip_space(b_o_s)) {
                                        U8StringView out = strip_start_ws(str);
                                        update_string(array, c + 1, out, str);
                                    }
                                    else if (!is_preserve_line(b_o_s))
                                    {
                                        U8StringView out = strip_first_line(str);
                                        update_string(array, c + 1, out, str);
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


    static void put(TinyObjectMap& map, const NamedCode& code, const Optional<bool>& val)
    {
        if (val.is_initialized()) {
            map.put(code, val.get());
        }
    }

    static void put_space_data(
            TinyObjectMap& map,
            const NamedCode& left_code,
            const NamedCode& right_code,
            const TplSpaceData& data
    ) {
        put(map, left_code, data.left_space);
        return put(map, right_code, data.right_space);
    }

    static Optional<bool> get_bool(const TinyObjectMap& map, const NamedCode& code)
    {
        Object val = map.get(code);
        if (val.is_not_null()) {
            return val.to_bool();
        }

        return {};
    }

    static Optional<bool> get_if_bottom_inner_space(const TinyObjectMap& map)
    {
        auto obj = map.get(BOTTOM_INNER_SPACE);
        if (obj.is_not_null()) {
            return obj.to_bool();
        }
        else {
            Object else_stmt = map.get(ELSE);
            if (else_stmt.is_not_null()) {
                return get_bool(else_stmt.as_tiny_object_map(), TOP_OUTER_SPACE);
            }

            return {};
        }
    }

    static Optional<bool> get_bottom_outer_space(const TinyObjectMap& map)
    {
        auto obj = map.get(BOTTOM_OUTER_SPACE);
        if (obj.is_not_null()) {
            return obj.to_bool();
        }
        else {
            Object else_stmt = map.get(ELSE);
            if (else_stmt.is_not_null()) {
                return get_bottom_outer_space(else_stmt.as_tiny_object_map());
            }
            else {
                return {};
            }
        }
    }

    static bool is_strip_space(int32_t code)
    {
        return code == IF_STMT.code() || code == FOR_STMT.code() || code == ELSE_STMT.code() ||
                code == VAR_STMT.code() || code == SET_STMT.code();
    }
};



struct TplForStatement: TemplateConstants {
    Optional<bool> top_outer_space;
    path::ast::IdentifierNode variable;
    Object expression;
    Optional<bool> top_inner_space;
    Object blocks;
    TplSpaceData bottom_space_data;

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(FOR_STMT);

        auto ctr = current_ctr();
        auto identifier = path::parser::HermesASTConverter::new_identifier(ctr, variable, false);

        map.put(VARIABLE, variable.identifier);
        map.put(EXPRESSION, expression);

        map.put(STATEMENTS, blocks);
        put_space_data(map, BOTTOM_INNER_SPACE, BOTTOM_OUTER_SPACE, bottom_space_data);

        put(map, TOP_OUTER_SPACE, top_outer_space);
        put(map, TOP_INNER_SPACE, top_inner_space);

        auto res = process_inner_space(blocks, top_inner_space, bottom_space_data.left_space);
        if (res.is_not_null()) {
            map.put(STATEMENTS, blocks);
        }

        process_outer_space(blocks);

        return map.as_object();
    }
};


struct TplSetStatement: TemplateConstants {
    Optional<bool> top_outer_space;
    path::ast::IdentifierNode variable;
    Object expression;
    Optional<bool> bottom_outer_space;

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(SET_STMT);

        map.put(VARIABLE, variable.identifier);
        map.put(EXPRESSION, expression);

        put(map, TOP_OUTER_SPACE, top_outer_space);
        put(map, BOTTOM_OUTER_SPACE, bottom_outer_space);

        return map.as_object();
    }
};

struct TplVarStatement: TemplateConstants {
    Object expression;

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(VAR_STMT);
        map.put(EXPRESSION, expression);
        return map.as_object();
    }
};


struct TplIfStatement;
struct TplElseStatement;

using TplIfAltBranch = boost::variant<
    TplSpaceData,
    boost::recursive_wrapper<TplIfStatement>,
    boost::recursive_wrapper<TplElseStatement>
>;

struct TplElseStatement: TemplateConstants {
    Optional<bool> top_outer_space;
    Object blocks;
    Optional<bool> top_inner_space;
    TplSpaceData bottom_space_data;

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(ELSE_STMT);
        map.put(STATEMENTS, blocks);

        put(map, TOP_OUTER_SPACE, top_outer_space);
        put(map, TOP_INNER_SPACE, top_inner_space);

        put_space_data(map, BOTTOM_INNER_SPACE, BOTTOM_OUTER_SPACE, bottom_space_data);

        process_inner_space(blocks, top_inner_space, bottom_space_data.left_space);
        process_outer_space(blocks);

        return map.as_object();
    }
};



struct TplIfStatement: TemplateConstants {
    Optional<bool> top_outer_space;
    Object expression;
    Optional<bool> top_inner_space;
    Object blocks;

    TplIfAltBranch alt_branch;

    struct AltOp {
        Object value;
        NamedCode prop{ELSE};
    };

    operator path::ast::HermesValueNode() const
    {
        auto map = new_ast_node(IF_STMT);
        map.put(EXPRESSION, expression);
        map.put(STATEMENTS, blocks);

        put(map, TOP_OUTER_SPACE, top_outer_space);
        put(map, TOP_INNER_SPACE, top_inner_space);

        if (alt_branch.which() == 0)
        {
            const TplSpaceData& bottom_space_data = boost::get<TplSpaceData>(alt_branch);
            put_space_data(map, BOTTOM_INNER_SPACE, BOTTOM_OUTER_SPACE, bottom_space_data);
        }
        else if (alt_branch.which() == 1)
        {
            const TplIfStatement& elif_stmt = boost::get<TplIfStatement>(alt_branch);
            map.put(ELSE, ((path::ast::HermesValueNode)elif_stmt).value);
        }
        else {
            const TplElseStatement& elif_stmt = boost::get<TplElseStatement>(alt_branch);
            map.put(ELSE, ((path::ast::HermesValueNode)elif_stmt).value);
        }

        auto bottom_inner_space = get_if_bottom_inner_space(map);
        process_inner_space(blocks, top_inner_space, bottom_inner_space);

        process_outer_space(blocks);

        return map.as_object();
    }
};



}



BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplForStatement,
    (memoria::Optional<bool>, top_outer_space)
    (memoria::hermes::path::ast::IdentifierNode, variable)
    (memoria::hermes::Object, expression)
    (memoria::Optional<bool>, top_inner_space)
    (memoria::hermes::Object, blocks)
    (memoria::hermes::TplSpaceData, bottom_space_data)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplSetStatement,
    (memoria::Optional<bool>, top_outer_space)
    (memoria::hermes::path::ast::IdentifierNode, variable)
    (memoria::hermes::Object, expression)
    (memoria::Optional<bool>, bottom_outer_space)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplVarStatement,
    (memoria::hermes::Object, expression)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplIfStatement,
    (memoria::Optional<bool>, top_outer_space)
    (memoria::hermes::Object, expression)
    (memoria::Optional<bool>, top_inner_space)
    (memoria::hermes::Object, blocks)
    (memoria::hermes::TplIfAltBranch, alt_branch)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplElseStatement,
    (memoria::Optional<bool>, top_outer_space)
    (memoria::Optional<bool>, top_inner_space)
    (memoria::hermes::Object, blocks)
    (memoria::hermes::TplSpaceData, bottom_space_data)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::TplSpaceData,
    (memoria::Optional<bool>, left_space)
    (memoria::Optional<bool>, right_space)
)
