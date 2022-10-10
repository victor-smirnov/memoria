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
#ifndef BRACKETSPECIFIERNODE_H
#define BRACKETSPECIFIERNODE_H
#include "variantnode.h"
#include <boost/fusion/include/adapt_struct.hpp>

namespace memoria::jmespath { namespace ast {

class ArrayItemNode;
class FlattenOperatorNode;
class SliceExpressionNode;
class ListWildcardNode;
class FilterExpressionNode;
/**
 * @brief The BracketSpecifierNode class represents a JMESPath bracket
 * specifier.
 */
class BracketSpecifierNode : public VariantNode<
        boost::recursive_wrapper<ArrayItemNode>,
        boost::recursive_wrapper<FlattenOperatorNode>,
        boost::recursive_wrapper<SliceExpressionNode>,
        boost::recursive_wrapper<ListWildcardNode>,
        boost::recursive_wrapper<FilterExpressionNode> >
{
public:
    /**
     * @brief Constructs an empty BracketSpecifierNode object.
     */
    BracketSpecifierNode();
    /**
     * @brief Constructs a BracketSpecifierNode object with the given
     * @a expression as its value.
     * @param[in] expression The node's child expression.
     */
    BracketSpecifierNode(const ValueType& expression);
    /**
     * @brief Returns whather this expression requires the projection of
     * subsequent expressions.
     * @return Returns true if projection is required, otherwise returns false.
     */
    bool isProjection() const;
    /**
     * @brief Reports whether the node should stop an ongoing projection or
     * not.
     * @return Returns true if the node should stop an ongoing projection,
     * otherwise returns false.
     */
    bool stopsProjection() const;

private:
    /**
     * @brief A virtual function used to pin vtable to a transaltion unit
     */
    virtual void anchor();
};
}} // namespace jmespath::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::jmespath::ast::BracketSpecifierNode,
    (memoria::jmespath::ast::BracketSpecifierNode::ValueType, value)
)
#endif // BRACKETSPECIFIERNODE_H
