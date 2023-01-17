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
#include <memoria/core/hermes/path/path.h>

#include <boost/hana.hpp>

namespace memoria::hermes::path::parser {

class HermesASTConverter: public interpreter::AbstractVisitor, public ASTCodes {

    using ASTNodePtr = TinyObjectMap;

    Optional<Object> context_;

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
            auto map = current_ctr().make_tiny_map();
            if (add_string_names_) {
                map.put(AST_NODE_NAME, NULL_NODE.name());
            }
            map.put_t<BigInt>(CODE_ATTR, NULL_NODE.code());
            context_ = map.as_object();
        }
        else {
            node->accept(this);
        }
    }


    ASTNodePtr new_ast_node(const NamedCode& code)
    {
        return new_ast_node(code, add_string_names_);
    }



    static ASTNodePtr new_ast_node(const NamedCode& code, bool add_string_names)
    {
        auto map = current_ctr().make_tiny_map(4);
        map.put_t<Integer>(CODE_ATTR, code.code());

        if (add_string_names) {
            map.put(AST_NODE_NAME, code.name());
        }

        return map;
    }


    virtual void visit(const ast::IdentifierNode* node)
    {
        auto ctr = current_ctr();
        context_ = new_identifier(ctr, *node, add_string_names_).as_object();
    }

    static ASTNodePtr new_identifier(HermesCtrView& ctr, const ast::IdentifierNode& node, bool add_node_name = false)
    {
        auto map = new_ast_node(node.CODE, add_node_name);
        map.put(IDENTIFIER_ATTR, node.identifier);
        return map;
    }

    virtual void visit(const ast::RawStringNode* node)
    {
        auto map = new_ast_node(node->CODE);
        map.put(RAW_STRING_ATTR, node->rawString);
        context_ = map.as_object();
    }

    virtual void visit(const ast::HermesValueNode* node)
    {
        auto map = new_ast_node(node->CODE);
        map.put(VALUE_ATTR, node->value);
        context_ = map.as_object();
    }

    void visit(const ast::BinaryExpressionNode* node, const NamedCode& code)
    {
        auto map = new_ast_node(code);

        map.put(IS_PROJECTION_ATTR, node->isProjection());
        map.put(STOPS_PROJECTION_ATTR, node->stopsProjection());

        visit(&node->leftExpression);
        if (context_.is_initialized()) {
            map.put(LEFT_EXPRESSION_ATTR, context_.get());
            clear_context();
        }

        visit(&node->rightExpression);
        if (context_.is_initialized()) {
            map.put(RIGHT_EXPRESSION_ATTR, context_.get());
            clear_context();
        }

        context_ = map.as_object();
    }

    virtual void visit(const ast::SubexpressionNode* node)
    {
        visit(node, node->CODE);
    }

    virtual void visit(const ast::IndexExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE);

        map.put(IS_PROJECTION_ATTR, node->isProjection());
        map.put(STOPS_PROJECTION_ATTR, node->stopsProjection());

        visit(&node->leftExpression);
        if (context_.is_initialized()) {
            map.put(LEFT_EXPRESSION_ATTR, context_.get());
            clear_context();
        }

        visit(&node->bracketSpecifier);
        if (context_.is_initialized()) {
            map.put(BRACKET_SPECIFIER_ATTR, context_.get());
            clear_context();
        }

        visit(&node->rightExpression);
        if (context_.is_initialized()) {
            map.put(RIGHT_EXPRESSION_ATTR, context_.get());
            clear_context();
        }

        context_ = map.as_object();
    }

    virtual void visit(const ast::ArrayItemNode* node)
    {
        auto map = new_ast_node(node->CODE);
        map.put_t<BigInt>(INDEX_ATTR, node->index.convert_to<int64_t>());
        context_ = map.as_object();
    }

    virtual void visit(const ast::FlattenOperatorNode* node)
    {
        auto map = new_ast_node(node->CODE);
        context_ = map.as_object();
    }

    virtual void visit(const ast::BracketSpecifierNode* node)
    {
        node->accept(this);
    }

    virtual void visit(const ast::SliceExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE);

        if (node->start.is_initialized()) {
            map.put(START_ATTR, node->start.get().convert_to<int64_t>());
        }

        if (node->stop.is_initialized()) {
            map.put(STOP_ATTR, node->stop.get().convert_to<int64_t>());
        }

        if (node->step.is_initialized()) {
            map.put(STEP_ATTR, node->step.get().convert_to<int64_t>());
        }

        context_ = map.as_object();
    }

    virtual void visit(const ast::ListWildcardNode* node)
    {
        auto map = new_ast_node(node->CODE);
        context_ = map.as_object();
    }

    virtual void visit(const ast::HashWildcardNode* node)
    {
        visit(node, node->CODE);
    }

    virtual void visit(const ast::MultiselectListNode* node)
    {
        auto map = new_ast_node(node->CODE);

        auto array = current_ctr().make_object_array();
        map.put(EXPRESSIONS_ATTR, array.as_object());

        for (auto& item: node->expressions)
        {
            clear_context();
            visit(&item);
            if (context_.is_initialized()) {
                array.push_back(context_.get());
            }
        }
        context_ = map.as_object();
    }

    virtual void visit(const ast::MultiselectHashNode* node)
    {
        auto map = new_ast_node(node->CODE);

        auto array = current_ctr().make_object_array();
        map.put(EXPRESSIONS_ATTR, array.as_object());

        for (auto& item: node->expressions)
        {
            auto kv_pair = current_ctr().make_tiny_map();
            array.push_back(kv_pair.as_object());

            clear_context();
            visit(&item.first);
            if (context_.is_initialized()) {
                kv_pair.put(FIRST_ATTR, context_.get());
            }

            clear_context();
            visit(&item.second);
            if (context_.is_initialized()) {
                kv_pair.put(SECOND_ATTR, context_.get());
            }
        }

        clear_context();
        context_ = map.as_object();
    }

    virtual void visit(const ast::NotExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE);

        visit(&node->expression);
        if (context_.is_initialized()) {
            map.put(EXPRESSION_ATTR, context_.get());
        }

        clear_context();
        context_ = map.as_object();
    }

    virtual void visit(const ast::ComparatorExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE);

        map.put(IS_PROJECTION_ATTR, node->isProjection());
        map.put(STOPS_PROJECTION_ATTR, node->stopsProjection());
        map.put(COMPARATOR_ATTR, (int64_t)node->comparator);

        visit(&node->leftExpression);
        if (context_.is_initialized()) {
            map.put(LEFT_EXPRESSION_ATTR, context_.get());
            clear_context();
        }

        visit(&node->rightExpression);
        if (context_.is_initialized()) {
            map.put(RIGHT_EXPRESSION_ATTR, context_.get());
            clear_context();
        }

        clear_context();

        context_ = map.as_object();
    }

    virtual void visit(const ast::OrExpressionNode* node)
    {
        visit(node, node->CODE);
    }

    virtual void visit(const ast::AndExpressionNode* node)
    {
        visit(node, node->CODE);
    }

    virtual void visit(const ast::ParenExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE);
        visit(&node->expression);
        if (context_.is_initialized()) {
            map.put(EXPRESSION_ATTR, context_.get());
        }

        clear_context();
        context_ = map.as_object();
    }

    virtual void visit(const ast::PipeExpressionNode* node)
    {
        visit(node, node->CODE);
    }

    virtual void visit(const ast::CurrentNode* node)
    {
        auto map = new_ast_node(node->CODE);
        context_ = map.as_object();
    }

    virtual void visit(const ast::FilterExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE);
        visit(&node->expression);
        if (context_.is_initialized()) {
            map.put(EXPRESSION_ATTR, context_.get());
        }

        clear_context();
        context_ = map.as_object();
    }

    virtual void visit(const ast::FunctionExpressionNode* node)
    {
        auto map = new_ast_node(node->CODE);

        map.put(FUNCTION_NAME_ATTR, node->functionName);

        auto array = current_ctr().make_object_array();
        map.put(ARGUMENTS_ATTR, array.as_object());

        for (auto& item: node->arguments)
        {
            clear_context();
            auto visitor = boost::hana::overload(
                // evaluate expressions and return their results
                [&, this](const ast::ExpressionNode& node) -> Optional<Object> {
                    this->visit(&node);
                    return context_;
                },
                // in case of expression argument nodes return the expression they
                // hold so it can be evaluated inside a function
                [&, this](const ast::ExpressionArgumentNode& node) -> Optional<Object> {
                    this->visit(&node);
                    return context_;
                },
                // ignore blank arguments
                [](const boost::blank&) -> Optional<Object> {
                    return {};
                }
            );
            auto result = boost::apply_visitor(visitor, item);
            if (result.is_initialized()) {
                array.push_back(result.get());
            }
            else {
                array.push_back(Object{});
            }
        }

        clear_context();
        context_ = map.as_object();
    }

    virtual void visit(const ast::ExpressionArgumentNode* node)
    {
        auto map = new_ast_node(node->CODE);

        visit(&node->expression);

        if (context_.is_initialized()) {
            map.put(EXPRESSION_ATTR, context_.get());
        }

        clear_context();
        context_ = map.as_object();
    }

    void clear_context() {
        context_ = {};
    }
};



}
