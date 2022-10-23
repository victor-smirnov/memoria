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

#include "binaryexpressionnode.h"
#include "bracketspecifiernode.h"
#include <boost/fusion/include/adapt_struct.hpp>

namespace memoria::hermes::path { namespace ast {

/**
 * @brief The IndexExpressionNode class represents a HermesPath index expression.
 */
class IndexExpressionNode : public BinaryExpressionNode
{
public:
    /**
     * @brief Construct an empty IndexExpressionNode object.
     */
    IndexExpressionNode();
    /**
     * @brief Constructs an IndexExpressionNode object with the given
     * @a bracketSpecifier and empty left and right expressions.
     * @param[in] bracketNode The bracket specifier node.
     */
    IndexExpressionNode(const BracketSpecifierNode& bracketNode);
    /**
     * @brief Constructs an IndexExpressionNode with given @a expression as its
     * left hand expression, @a subexpression as its right hand expression and
     * with the given @a bracketSpecifier.
     * @param[in] left The left hand expression of the node.
     * @param[in] bracketNode The index expression's bracket specifier.
     * @param[in] right The right hand expression of the node.
     */
    IndexExpressionNode(const ExpressionNode& left,
                        const BracketSpecifierNode& bracketNode,
                        const ExpressionNode& right = {});
    /**
     * @brief Equality compares this node to the @a other
     * @param[in] other The node that should be compared.
     * @return Returns true if this object is equal to the @a other, otherwise
     * false
     */
    bool operator ==(const IndexExpressionNode& other) const;
    /**
     * @brief Returns whather this expression requires the projection of
     * subsequent expressions.
     * @return Returns true if projection is required, otherwise returns false.
     */
    bool isProjection() const override;
    /**
     * @brief Reports whether the node should stop an ongoing projection or
     * not.
     * @return Returns true if the node should stop an ongoing projection,
     * otherwise returns false.
     */
    bool stopsProjection() const override;
    /**
     * @brief Calls the visit method of the given @a visitor with the
     * dynamic type of the node.
     * @param[in] visitor A visitor implementation
     */
    void accept(interpreter::AbstractVisitor* visitor) const override;
    /**
     * @brief The bracket specifier in an index expression.
     */
    BracketSpecifierNode bracketSpecifier;
};
}} // namespace hermes::path::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::IndexExpressionNode,
    (memoria::hermes::path::ast::ExpressionNode, leftExpression)
    (memoria::hermes::path::ast::BracketSpecifierNode, bracketSpecifier)
    (memoria::hermes::path::ast::ExpressionNode, rightExpression)
)
