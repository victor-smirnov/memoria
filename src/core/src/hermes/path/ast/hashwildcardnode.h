/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
**
** This file is part of the jmespath.cpp project which is distributed under
** the MIT License (MIT).
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
#ifndef HASHWILDCARDNODE_H
#define HASHWILDCARDNODE_H
#include "binaryexpressionnode.h"

namespace memoria::jmespath { namespace ast {

/**
 * @brief The HashWildcardNode class represents a JMESPath hash wildcard
 * expression.
 */
class HashWildcardNode : public BinaryExpressionNode
{
public:
    /**
     * @brief Constructs an empty HashWildcardNode object.
     */
    HashWildcardNode();
    /**
     * @brief Constructs a HashWildcardNode object with the given
     * @a leftExpression and @a rightExpression as its children.
     * @param[in] leftExpression Left hand expression of the node.
     * @param[in] rightExpression Right hand expression of the node.
     */
    HashWildcardNode(const ExpressionNode& leftExpression,
                     const ExpressionNode& rightExpression);
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
};
}} // namespace jmespath::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::jmespath::ast::HashWildcardNode,
    (memoria::jmespath::ast::ExpressionNode, leftExpression)
    (memoria::jmespath::ast::ExpressionNode, rightExpression)
)
#endif // HASHWILDCARDNODE_H