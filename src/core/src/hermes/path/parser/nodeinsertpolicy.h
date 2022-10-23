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
#ifndef NODEINSERTPOLICY_H
#define NODEINSERTPOLICY_H
#include "../ast/allnodes.h"

namespace memoria::hermes::path { namespace parser {

/**
 * @brief The NodeInsertPolicy class is a functor for inserting a given node
 * into the AST.
 */
class NodeInsertPolicy
{
public:
    /**
     * @brief The result type of the functor.
     */
    using result_type = void;

    /**
     * @brief Inserts the given @a node into the AST at the location of the
     * @a targetNode.
     * @param[in] targetNode The node located where @a node should be inserted.
     * @param[in] node The node that will be inserted.
     * @{
     */
    template <typename T, typename
        std::enable_if<
            std::is_base_of<ast::BinaryExpressionNode, T>::value
            && !std::is_same<ast::SubexpressionNode, T>::value, int>::type = 0>
    void operator()(ast::ExpressionNode& targetNode,
                    T& node) const
    {
        node.rightExpression = targetNode;
        targetNode = node;
    }

    template <typename T, typename
        std::enable_if<
            std::is_same<ast::NotExpressionNode, T>::value, int>::type = 0>
    void operator()(ast::ExpressionNode& targetNode,
                    T& node) const
    {
        node.expression = targetNode;
        targetNode = node;
    }

    template <typename T, typename
        std::enable_if<
            (!std::is_base_of<ast::BinaryExpressionNode, T>::value
            && !std::is_same<ast::NotExpressionNode, T>::value
            && std::is_assignable<ast::ExpressionNode, T>::value)
            || std::is_same<ast::SubexpressionNode, T>::value, int>::type = 0>
    void operator()(ast::ExpressionNode& targetNode,
                    T& node) const
    {
        targetNode = node;
    }
    /** @}*/
};
}} // namespace hermes::path::parser
#endif // NODEINSERTPOLICY_H
