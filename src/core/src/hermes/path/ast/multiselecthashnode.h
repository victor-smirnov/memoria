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
#ifndef MULTISELECTHASHNODE_H
#define MULTISELECTHASHNODE_H
#include "abstractnode.h"
#include "identifiernode.h"
#include "expressionnode.h"
#include <utility>
#include <initializer_list>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

namespace memoria::hermes::path { namespace ast {

/**
 * @brief The MultiselectHashNode class represents a JMESPath multiselect
 * hash expression.
 */
class MultiselectHashNode : public AbstractNode
{
public:
    using KeyValuePairType = std::pair<IdentifierNode, ExpressionNode>;
    /**
     * @brief Constructs an empty MultiselectHashNode object.
     */
    MultiselectHashNode();
    /**
     * @brief Constructs a MultiselectHashNode object with the given
     * @a expressions as its subexpressions.
     * @param[in] subexpressions The node's subexpressions.
     */
    MultiselectHashNode(const std::vector<KeyValuePairType>& subexpressions);
    /**
     * @brief Constructs a MultiselectHashNode object with the given
     * @a expressions as its subexpressions.
     * @param[in] subexpressions The node's subexpressions.
     */
    MultiselectHashNode(const std::initializer_list<KeyValuePairType>& subexpressions);
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
    bool operator==(const MultiselectHashNode& other) const;
    /**
     * @brief The node's child expressions.
     */
    std::vector<KeyValuePairType> expressions;
};
}} // namespace hermes::path::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::MultiselectHashNode,
    (std::vector<memoria::hermes::path::ast::MultiselectHashNode::KeyValuePairType>,
     expressions)
)
#endif // MULTISELECTHASHNODE_H
