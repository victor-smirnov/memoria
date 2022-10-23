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
#ifndef SUBEXPRESSIONNODE_H
#define SUBEXPRESSIONNODE_H
#include "binaryexpressionnode.h"

namespace memoria::hermes::path { namespace ast {

/**
 * @brief The SubexpressionNode class represents a HermesPath subexpression.
 */
class SubexpressionNode : public BinaryExpressionNode
{
public:
    /**
     * @brief Constructs an empty SubexpressionNode object.
     */
    SubexpressionNode();
    /**
     * @brief Constructs an SubexpressionNode object with the given left hand
     * side @a expression and a right hand side @a subexpression.
     * @param[in] expression The node's left hand side child expression
     * @param[in] subexpression The node's right hand side child expression
     */
    SubexpressionNode(const ExpressionNode& expression,
                      const ExpressionNode& subexpression = {});
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
};
}} // namespace hermes::path::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::SubexpressionNode,
    (memoria::hermes::path::ast::ExpressionNode, leftExpression)
    (memoria::hermes::path::ast::ExpressionNode, rightExpression)
)
#endif // SUBEXPRESSIONNODE_H
