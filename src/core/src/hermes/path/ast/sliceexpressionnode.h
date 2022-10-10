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
#ifndef SLICEEXPRESSIONNODE_H
#define SLICEEXPRESSIONNODE_H
#include "abstractnode.h"
#include "memoria/core/hermes/path/types.h"
#include "../interpreter/abstractvisitor.h"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/optional.hpp>

namespace memoria::jmespath { namespace ast {

/**
 * @brief The SliceExpressionNode class represents a JMESPath slice expression.
 */
class SliceExpressionNode : public AbstractNode
{
public:
    using IndexType = boost::optional<Index>;
    /**
     * @brief Constructs a SliceExpressionNode object with the given index
     * values.
     * @param[in] startIndex Inclusive start index of slice.
     * @param[in] stopIndex Exclusive end index of slice.
     * @param[in] stepIndex Step index of slice.
     */
    SliceExpressionNode(const IndexType& startIndex = boost::none,
                        const IndexType& stopIndex = boost::none,
                        const IndexType& stepIndex = boost::none);
    /**
     * @brief Calls the visit method of the given @a visitor with the
     * dynamic type of the node.
     * @param visitor A visitor implementation
     */
    void accept(interpreter::AbstractVisitor* visitor) const override;
    /**
     * @brief Equality compares this node to the @a other
     * @param other The node that should be compared.
     * @return Returns true if this object is equal to the @a other, otherwise
     * false
     */
    bool operator==(const SliceExpressionNode& other) const;
    /**
     * @brief Inclusive start index.
     */
    IndexType start;
    /**
     * @brief Exclusive end index.
     */
    IndexType stop;
    /**
     * @brief Step index.
     */
    IndexType step;
};
}} // namespace jmespath::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::jmespath::ast::SliceExpressionNode,
    (memoria::jmespath::ast::SliceExpressionNode::IndexType, start)
    (memoria::jmespath::ast::SliceExpressionNode::IndexType, stop)
    (memoria::jmespath::ast::SliceExpressionNode::IndexType, step)
)
#endif // SLICEEXPRESSIONNODE_H
