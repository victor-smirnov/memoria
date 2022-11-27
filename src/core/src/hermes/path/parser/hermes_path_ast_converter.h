/****************************************************************************
**
** Author: Victor Smirnov
** Copyright (c) 2022 Victor Smirnov
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
****************************************************************************/

#pragma once

#include "../interpreter/abstractvisitor.h"
#include "../ast/allnodes.h"

#include "../../hermes_ctr_builder.hpp"

#include <memoria/core/tools/optional.hpp>

#include <boost/hana.hpp>

namespace memoria::hermes::path::parser {

class HermesASTConverter: public interpreter::AbstractVisitor, public ast::ExpressionAttrs {
    Optional<ObjectPtr> context_;

    static constexpr const U8StringView AST_NODE_NAME_ATTR              = "astNodeName";
    static constexpr const U8StringView IDENTIFIER_NODE_NAME            = "Identifier";
    static constexpr const U8StringView RAW_STRING_NODE_NAME            = "RawString";
    static constexpr const U8StringView HERMES_VALUE_NODE_NAME          = "HermesValue";
    static constexpr const U8StringView SUBEXPRESSION_NODE_NAME         = "Subexpression";
    static constexpr const U8StringView INDEX_EXPRESSION_NODE_NAME      = "IndexEXpression";
    static constexpr const U8StringView ARRAY_ITEM_NODE_NAME            = "ArrayItem";
    static constexpr const U8StringView FLATTEN_OPERATOR_NODE_NAME      = "FlattenExpression";
    static constexpr const U8StringView SLICE_EXPRESSION_NODE_NAME      = "SliceExpression";
    static constexpr const U8StringView LIST_WILDCARD_NODE_NAME         = "ListWildcard";
    static constexpr const U8StringView HASH_WILDCARD_NODE_NAME         = "HashWildcard";
    static constexpr const U8StringView MULTISELECT_LIST_NODE_NAME      = "MultiselectList";
    static constexpr const U8StringView MULTISELECT_HASH_NODE_NAME      = "MutiselectHash";
    static constexpr const U8StringView NOT_EXPRESSION_NODE_NAME        = "NotExpression";
    static constexpr const U8StringView OR_EXPRESSION_NODE_NAME         = "OrExpression";
    static constexpr const U8StringView COMPARATOR_EXPRESSION_NODE_NAME = "ComparatorExpression";
    static constexpr const U8StringView AND_EXPRESSION_NODE_NAME        = "AndExpression";
    static constexpr const U8StringView PAREN_EXPRESSION_NODE_NAME      = "ParenExpression";
    static constexpr const U8StringView CURRENT_NODE_NAME               = "Current";
    static constexpr const U8StringView FILTER_EXPRESSION_NODE_NAME     = "FilterExpression";
    static constexpr const U8StringView PIPE_EXPRESSION_NODE_NAME       = "PipeExpression";
    static constexpr const U8StringView FUNCTION_EXPRESSION_NODE_NAME   = "FunctionExpression";
    static constexpr const U8StringView EXPRESSION_ARGUMENT_NODE_NAME   = "ExpressionArgument";


    bool add_string_names_;
public:
    HermesASTConverter(bool add_string_names = true):
        add_string_names_(add_string_names)
    {}

    auto& context() {
        return context_;
    }

    virtual void visit(const ast::AbstractNode* node) {
        node->accept(this);
    }

    virtual void visit(const ast::ExpressionNode* node)
    {
        if (node->isNull())
        {
            auto map = current_ctr()->new_map();
            if (add_string_names_) {
                map->put_dataobject<Varchar>(AST_NODE_NAME_ATTR, "NullNode");
            }
            map->put_dataobject<BigInt>(CODE_ATTR, node->NULL_NODE_CODE);
            context_ = map->as_object();
        }
        else {
            node->accept(this);
        }
    }

    ObjectMapPtr new_ast_node(int64_t code, U8StringView name)
    {
        return new_ast_node(code, name, add_string_names_);
    }

    static ObjectMapPtr new_ast_node(int64_t code, U8StringView name, bool add_string_names)
    {
        auto map = current_ctr()->new_map();
        map->put_dataobject<BigInt>(CODE_ATTR, code);

        if (add_string_names) {
            map->put_dataobject<Varchar>(AST_NODE_NAME_ATTR, name);
        }

        return map;
    }

    virtual void visit(const ast::IdentifierNode* node)
    {
        context_ = new_identifier(*current_ctr(), *node, add_string_names_)->as_object();
    }

    static ObjectMapPtr new_identifier(HermesCtr& ctr, const ast::IdentifierNode& node, bool add_node_name = false)
    {
        auto map = new_ast_node(node.CODE, IDENTIFIER_NODE_NAME, add_node_name);
        map->put_dataobject<Varchar>(IDENTIFIER, node.identifier);
        return map;
    }

    virtual void visit(const ast::RawStringNode* node)
    {
        auto map = new_ast_node(node->CODE, RAW_STRING_NODE_NAME);
        map->put_dataobject<Varchar>(RAW_STRING, node->rawString);
        context_ = map->as_object();
    }

    virtual void visit(const ast::HermesValueNode* node)
    {
        auto map = new_ast_node(node->CODE, HERMES_VALUE_NODE_NAME);
        map->put(VALUE, node->value);
        context_ = map->as_object();
    }

    void visit(const ast::BinaryExpressionNode* node, int64_t code, const U8StringView& ast_node_name)
    {
        auto map = new_ast_node(code, ast_node_name);

        map->put_dataobject<Boolean>(IS_PROJECTION, node->isProjection());
        map->put_dataobject<Boolean>(STOPS_PROJECTION, node->stopsProjection());

        visit(&node->leftExpression);
        if (context_.is_initialized()) {
            map->put(LEFT_EXPRESSION, context_.get());
            clear_context();
        }

        visit(&node->rightExpression);
        if (context_.is_initialized()) {
            map->put(RIGHT_EXPRESSION, context_.get());
            clear_context();
        }

        context_ = map->as_object();
    }

    virtual void visit(const ast::SubexpressionNode* node)
    {
        visit(node, node->CODE, SUBEXPRESSION_NODE_NAME);
    }

    virtual void visit(const ast::IndexExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE, INDEX_EXPRESSION_NODE_NAME);

        map->put_dataobject<Boolean>(IS_PROJECTION, node->isProjection());
        map->put_dataobject<Boolean>(STOPS_PROJECTION, node->stopsProjection());

        visit(&node->leftExpression);
        if (context_.is_initialized()) {
            map->put(LEFT_EXPRESSION, context_.get());
            clear_context();
        }

        visit(&node->bracketSpecifier);
        if (context_.is_initialized()) {
            map->put(BRACKET_SPECIFIER, context_.get());
            clear_context();
        }

        visit(&node->rightExpression);
        if (context_.is_initialized()) {
            map->put(RIGHT_EXPRESSION, context_.get());
            clear_context();
        }

        context_ = map->as_object();
    }

    virtual void visit(const ast::ArrayItemNode* node)
    {
        auto map = new_ast_node(node->CODE, ARRAY_ITEM_NODE_NAME);
        map->put_dataobject<BigInt>(INDEX, node->index.convert_to<int64_t>());
        context_ = map->as_object();
    }

    virtual void visit(const ast::FlattenOperatorNode* node)
    {
        auto map = new_ast_node(node->CODE, FLATTEN_OPERATOR_NODE_NAME);
        context_ = map->as_object();
    }

    virtual void visit(const ast::BracketSpecifierNode* node)
    {
        node->accept(this);
    }

    virtual void visit(const ast::SliceExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE, SLICE_EXPRESSION_NODE_NAME);

        if (node->start.is_initialized()) {
            map->put_dataobject<BigInt>(START, node->start.get().convert_to<int64_t>());
        }

        if (node->stop.is_initialized()) {
            map->put_dataobject<BigInt>(STOP, node->stop.get().convert_to<int64_t>());
        }

        if (node->step.is_initialized()) {
            map->put_dataobject<BigInt>(STEP, node->step.get().convert_to<int64_t>());
        }

        context_ = map->as_object();
    }

    virtual void visit(const ast::ListWildcardNode* node)
    {
        auto map = new_ast_node(node->CODE, LIST_WILDCARD_NODE_NAME);
        context_ = map->as_object();
    }

    virtual void visit(const ast::HashWildcardNode* node)
    {
        visit(node, node->CODE, HASH_WILDCARD_NODE_NAME);
    }

    virtual void visit(const ast::MultiselectListNode* node)
    {
        auto map = new_ast_node(node->CODE, MULTISELECT_LIST_NODE_NAME);

        auto array = current_ctr()->new_array();
        map->put(EXPRESSIONS, array->as_object());

        for (auto& item: node->expressions)
        {
            clear_context();
            visit(&item);
            if (context_.is_initialized()) {
                array->append(context_.get());
            }
        }
        context_ = map->as_object();
    }

    virtual void visit(const ast::MultiselectHashNode* node)
    {
        auto map = new_ast_node(node->CODE, MULTISELECT_HASH_NODE_NAME);

        auto array = current_ctr()->new_array();
        map->put(EXPRESSIONS, array->as_object());

        for (auto& item: node->expressions)
        {
            auto kv_pair = current_ctr()->new_map();
            array->append(kv_pair->as_object());

            clear_context();
            visit(&item.first);
            if (context_.is_initialized()) {
                kv_pair->put(FIRST, context_.get());
            }

            clear_context();
            visit(&item.second);
            if (context_.is_initialized()) {
                kv_pair->put(SECOND, context_.get());
            }
        }

        clear_context();
        context_ = map->as_object();
    }

    virtual void visit(const ast::NotExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE, NOT_EXPRESSION_NODE_NAME);

        visit(&node->expression);
        if (context_.is_initialized()) {
            map->put(EXPRESSION, context_.get());
        }

        clear_context();
        context_ = map->as_object();
    }

    virtual void visit(const ast::ComparatorExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE, COMPARATOR_EXPRESSION_NODE_NAME);

        map->put_dataobject<Boolean>(IS_PROJECTION, node->isProjection());
        map->put_dataobject<Boolean>(STOPS_PROJECTION, node->stopsProjection());
        map->put_dataobject<BigInt>(COMPARATOR, (int64_t)node->comparator);

        visit(&node->leftExpression);
        if (context_.is_initialized()) {
            map->put(LEFT_EXPRESSION, context_.get());
            clear_context();
        }

        visit(&node->rightExpression);
        if (context_.is_initialized()) {
            map->put(RIGHT_EXPRESSION, context_.get());
            clear_context();
        }

        clear_context();

        context_ = map->as_object();
    }

    virtual void visit(const ast::OrExpressionNode* node)
    {
        visit(node, node->CODE, OR_EXPRESSION_NODE_NAME);
    }

    virtual void visit(const ast::AndExpressionNode* node)
    {
        visit(node, node->CODE, AND_EXPRESSION_NODE_NAME);
    }

    virtual void visit(const ast::ParenExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE, PAREN_EXPRESSION_NODE_NAME);
        visit(&node->expression);
        if (context_.is_initialized()) {
            map->put(EXPRESSION, context_.get());
        }

        clear_context();
        context_ = map->as_object();
    }

    virtual void visit(const ast::PipeExpressionNode* node)
    {
        visit(node, node->CODE, PIPE_EXPRESSION_NODE_NAME);
    }

    virtual void visit(const ast::CurrentNode* node)
    {
        auto map = new_ast_node(node->CODE, CURRENT_NODE_NAME);
        context_ = map->as_object();
    }

    virtual void visit(const ast::FilterExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE, FILTER_EXPRESSION_NODE_NAME);
        visit(&node->expression);
        if (context_.is_initialized()) {
            map->put(EXPRESSION, context_.get());
        }

        clear_context();
        context_ = map->as_object();
    }

    virtual void visit(const ast::FunctionExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE, FUNCTION_EXPRESSION_NODE_NAME);

        map->put_dataobject<Varchar>(FUNCTION_NAME, node->functionName);

        auto array = current_ctr()->new_array();
        map->put(ARGUMENTS, array->as_object());

        for (auto& item: node->arguments)
        {
            clear_context();
            auto visitor = boost::hana::overload(
                // evaluate expressions and return their results
                [&, this](const ast::ExpressionNode& node) -> Optional<ObjectPtr> {
                    this->visit(&node);
                    return context_;
                },
                // in case of expression argument nodes return the expression they
                // hold so it can be evaluated inside a function
                [&, this](const ast::ExpressionArgumentNode& node) -> Optional<ObjectPtr> {
                    this->visit(&node);
                    return context_;
                },
                // ignore blank arguments
                [](const boost::blank&) -> Optional<ObjectPtr> {
                    return {};
                }
            );
            auto result = boost::apply_visitor(visitor, item);
            if (result.is_initialized()) {
                array->append(result.get());
            }
            else {
                array->append(ObjectPtr{});
            }
        }

        clear_context();
        context_ = map->as_object();
    }

    virtual void visit(const ast::ExpressionArgumentNode* node)
    {
        auto map = new_ast_node(node->CODE, EXPRESSION_ARGUMENT_NODE_NAME);

        visit(&node->expression);

        if (context_.is_initialized()) {
            map->put(EXPRESSION, context_.get());
        }

        clear_context();
        context_ = map->as_object();
    }

    void clear_context() {
        context_ = {};
    }
};



}
