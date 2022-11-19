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

#include "abstractnode.h"
#include "memoria/core/hermes/path/types.h"
#include <vector>
#include <initializer_list>
#include <boost/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace memoria::hermes::path { namespace ast {

class ExpressionNode;
class ExpressionArgumentNode;
/**
 * @brief The FunctionExpressionNode class represents a HermesPath function
 * expression.
 */
class FunctionExpressionNode : public AbstractNode
{
public:
    using ArgumentType = boost::variant<boost::blank,
        boost::recursive_wrapper<ExpressionNode>,
        boost::recursive_wrapper<ExpressionArgumentNode> >;
    /**
     * @brief Constructs an empty FunctionExpressionNode object.
     */
    FunctionExpressionNode();
    /**
     * @brief Constructs a FunctionExpressionNode object with the given
     * @a function name and list of @a arguments as its arguments
     * @param[in] name The function's identifier
     * @param[in] argumentList The function's arguments
     */
    FunctionExpressionNode(const String& name,
        const std::initializer_list<ArgumentType>& argumentList = {});
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
    bool operator==(const FunctionExpressionNode& other) const;
    /**
     * @brief The function's name.
     */
    String functionName;
    /**
     * @brief The function expressions's arguments.
     */
    std::vector<ArgumentType> arguments;

    static constexpr int64_t CODE = 10;
};
}} // namespace hermes::path::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::FunctionExpressionNode,
    (memoria::hermes::path::String, functionName)
    (std::vector<memoria::hermes::path::ast::FunctionExpressionNode::ArgumentType>,
     arguments)
)
