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
#ifndef ARRAYITEMNODE_H
#define ARRAYITEMNODE_H
#include "abstractnode.h"
#include "memoria/core/hermes/path/types.h"
#include <boost/fusion/include/adapt_struct.hpp>

namespace memoria::hermes::path { namespace ast {

/**
 * @brief The ArrayItemNode class represents a JMESPath array index
 * expression.
 */
class ArrayItemNode : public AbstractNode
{
public:
    /**
     * @brief Constructs an empty ArrayItemNode object.
     */
    ArrayItemNode();
    /**
     * @brief Constructs an ArrayItemNode object with the given @a index as its
     * value.
     * @param[in] itemIndex The node's value.
     */
    ArrayItemNode(Index itemIndex);
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
    bool operator==(const ArrayItemNode& other) const;
    /**
     * @brief The node's value.
     */
    Index index;
};
}} // namespace hermes::path::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::ArrayItemNode,
    (memoria::hermes::path::Index, index)
)
#endif // ARRAYITEMNODE_H
