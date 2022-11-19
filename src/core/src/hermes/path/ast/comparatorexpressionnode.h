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
#include <boost/fusion/include/adapt_struct.hpp>

namespace memoria::hermes::path { namespace ast {

/**
 * @brief The ComparatorExpressionNode class represents a HermesPath comparator
 * expression.
 */
class ComparatorExpressionNode : public BinaryExpressionNode
{
public:
    /**
     * @brief The Comparator enum defines the available comparison operators.
     */
    enum class Comparator
    {
        Unknown,
        Less,
        LessOrEqual,
        Equal,
        GreaterOrEqual,
        Greater,
        NotEqual
    };
    /**
     * @brief Constructs an empty ComparatorExpressionNode object.
     */
    ComparatorExpressionNode();
    /**
     * @brief Constructs a ComparatorExpressionNode object with the given @a
     * leftExpression, @a comparator and @a rightExpression.
     * @param[in] left The node's left hand child expression.
     * @param[in] valueComparator The type of comparison operator to use for
     * comparing the results of the left and right hand child expressions.
     * @param[in] right The node's right hand child expression.
     */
    ComparatorExpressionNode(const ExpressionNode& left,
                             Comparator valueComparator,
                             const ExpressionNode& right);
    /**
     * @brief Equality compares this node to the @a other
     * @param[in] other The node that should be compared.
     * @return Returns true if this object is equal to the @a other, otherwise
     * false
     */
    bool operator ==(const ComparatorExpressionNode& other) const;
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
     * @brief The type of comparator associated with the expression.
     */
    Comparator comparator;

    static constexpr int64_t CODE = 5;
};
}} // namespace hermes::path::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::ComparatorExpressionNode,
    (memoria::hermes::path::ast::ExpressionNode, leftExpression)
    (memoria::hermes::path::ast::ComparatorExpressionNode::Comparator, comparator)
    (memoria::hermes::path::ast::ExpressionNode, rightExpression)
)

