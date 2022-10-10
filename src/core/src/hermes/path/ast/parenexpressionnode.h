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
#ifndef PARENEXPRESSIONNODE_H
#define PARENEXPRESSIONNODE_H
#include "expressionnode.h"
#include <boost/fusion/include/adapt_struct.hpp>

namespace memoria::jmespath { namespace ast {

/**
 * @brief The ParenExpressionNode class represents a JMESPath paren expression.
 */
class ParenExpressionNode : public AbstractNode
{
public:
    /**
     * @brief Constructs an emtpy ParenExpressionNode object.
     */
    ParenExpressionNode();
    /**
     * @brief Constructs a ParenExpressionNode object with the given @a expression
     * as its child expression.
     * @param[in] subexpression The node's child expression.
     */
    ParenExpressionNode(const ExpressionNode& subexpression);
    /**
     * @brief Calls the visit method of the given @a visitor with the
     * dynamic type of the node.
     * @param[in] visitor A visitor implementation
     */
    void accept(interpreter::AbstractVisitor* visitor) const override;
    /**
     * @brief Equality compares this node to the @a other
     * @param[in] other The node that should be compared.
     * @return Returns true if this object is equal to the @a other, otherwise
     * false
     */
    bool operator==(const ParenExpressionNode& other) const;
    /**
     * @brief The node's child expression.
     */
    ExpressionNode expression;
};
}} // namespace jmespath::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::jmespath::ast::ParenExpressionNode,
    (memoria::jmespath::ast::ExpressionNode, expression)
)
#endif // PARENEXPRESSIONNODE_H
