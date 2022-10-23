/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
**
** This file is originally based on the jmespath.cpp project
** (https://github.com/robertmrk/jmespath.cpp, commitid: 9c9702a)
** and is distributed under the MIT License (MIT).
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

namespace memoria::hermes::path { namespace ast {

class AbstractNode;
class Node;
class ExpressionNode;
class IdentifierNode;
class RawStringNode;
class LiteralNode;
class SubexpressionNode;
class IndexExpressionNode;
class ArrayItemNode;
class FlattenOperatorNode;
class BracketSpecifierNode;
class SliceExpressionNode;
class ListWildcardNode;
class HashWildcardNode;
class MultiselectListNode;
class MultiselectHashNode;
class NotExpressionNode;
class ComparatorExpressionNode;
class OrExpressionNode;
class AndExpressionNode;
class ParenExpressionNode;
class PipeExpressionNode;
class CurrentNode;
class FilterExpressionNode;
class FunctionExpressionNode;
class ExpressionArgumentNode;
}} // namespace hermes::path::ast

/**
 * @namespace hermes::path::interpreter
 * @brief Classes for interpreting the AST of the HermesPath expression.
 */
namespace memoria::hermes::path { namespace interpreter {
/**
 * @brief The AbstractVisitor class is an interface which
 * defines the member functions required to visit every
 * type of AST node
 */
class AbstractVisitor
{
public:
    /**
     * @brief Destroys the AbstractVisitor object.
     */
    virtual ~AbstractVisitor();
    /**
     * @brief Visits the given @a node.
     * @param[in] node Pointer to the node
     * @{
     */
    virtual void visit(const ast::AbstractNode* node) = 0;
    virtual void visit(const ast::ExpressionNode* node) = 0;
    virtual void visit(const ast::IdentifierNode* node) = 0;
    virtual void visit(const ast::RawStringNode* node) = 0;
    virtual void visit(const ast::LiteralNode* node) = 0;
    virtual void visit(const ast::SubexpressionNode* node) = 0;
    virtual void visit(const ast::IndexExpressionNode* node) = 0;
    virtual void visit(const ast::ArrayItemNode* node) = 0;
    virtual void visit(const ast::FlattenOperatorNode* node) = 0;
    virtual void visit(const ast::BracketSpecifierNode* node) = 0;
    virtual void visit(const ast::SliceExpressionNode* node) = 0;
    virtual void visit(const ast::ListWildcardNode* node) = 0;
    virtual void visit(const ast::HashWildcardNode* node) = 0;
    virtual void visit(const ast::MultiselectListNode* node) = 0;
    virtual void visit(const ast::MultiselectHashNode* node) = 0;
    virtual void visit(const ast::NotExpressionNode* node) = 0;
    virtual void visit(const ast::ComparatorExpressionNode* node) = 0;
    virtual void visit(const ast::OrExpressionNode* node) = 0;
    virtual void visit(const ast::AndExpressionNode* node) = 0;
    virtual void visit(const ast::ParenExpressionNode* node) = 0;
    virtual void visit(const ast::PipeExpressionNode* node) = 0;
    virtual void visit(const ast::CurrentNode* node) = 0;
    virtual void visit(const ast::FilterExpressionNode* node) = 0;
    virtual void visit(const ast::FunctionExpressionNode* node) = 0;
    virtual void visit(const ast::ExpressionArgumentNode* node) = 0;
    /** @}*/
};
}} // namespace hermes::path::interpreter
