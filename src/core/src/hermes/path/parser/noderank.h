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
#ifndef NODERANK_H
#define NODERANK_H
#include "../ast/allnodes.h"

namespace memoria::jmespath { namespace parser {

/**
 * @brief Returns the rank of the given @a node object's type.
 * @tparam T The type of the @a node object.
 * @return Returns the rank of the node as an integer.
 */
template <typename T>
inline int nodeRank(const T&)
{
    return 0;
}

/**
 * @brief Returns the rank of the node object's type contained in @a variant.
 * @param[in] variant A variant object containing a @ref ast::AbstractNode
 * @return Returns the rank of the node as an integer.
 */
template <typename... Args>
inline int nodeRank(const boost::variant<Args...>& variant)
{
    return boost::apply_visitor([](const auto& node){
        return nodeRank(node);
    }, variant);
}

/**
 * @brief Returns the rank of the given @a node object's type.
 * @param[in] node A node object
 * @return Returns the rank of the node as an integer.
 * @{
 */
template <>
inline int nodeRank(const ast::ExpressionNode& node)
{
    if(node.isNull())
    {
        return -1;
    }
    else
    {
        return nodeRank(node.value);
    }
}

template <>
inline int nodeRank(const ast::SubexpressionNode&)
{
    return 1;
}

template <>
inline int nodeRank(const ast::BracketSpecifierNode& node)
{
    return nodeRank(node.value);
}

template <>
inline int nodeRank(const ast::IndexExpressionNode& node)
{
    return nodeRank(node.bracketSpecifier);
}

template <>
inline int nodeRank(const ast::ArrayItemNode&)
{
    return 1;
}

template <>
inline int nodeRank(const ast::FlattenOperatorNode&)
{
    return 2;
}

template <>
inline int nodeRank(const ast::SliceExpressionNode&)
{
    return 2;
}

template <>
inline int nodeRank(const ast::ListWildcardNode&)
{
    return 2;
}

template <>
inline int nodeRank(const ast::HashWildcardNode&)
{
    return 2;
}

template <>
inline int nodeRank(const ast::FilterExpressionNode&)
{
    return 2;
}

template <>
inline int nodeRank(const ast::NotExpressionNode&)
{
    return 3;
}

template <>
inline int nodeRank(const ast::ComparatorExpressionNode&)
{
    return 4;
}

template <>
inline int nodeRank(const ast::AndExpressionNode&)
{
    return 5;
}

template <>
inline int nodeRank(const ast::OrExpressionNode&)
{
    return 6;
}

template <>
inline int nodeRank(const ast::PipeExpressionNode&)
{
    return 7;
}
/** @} */
}} // namespace jmespath::parser
#endif // NODERANK_H
