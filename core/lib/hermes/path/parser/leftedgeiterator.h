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

#include "../ast/expressionnode.h"
#include "leftchildextractor.h"
#include <iterator>

namespace memoria::hermes::path { namespace parser {

/**
 * @brief The LeftEdgeIterator class is a forward iterator which can be used to
 * iterate over the ExpressionNode objects along the left edge of the AST.
 */
class LeftEdgeIterator
{
    using NodeT = ast::ExpressionNode;
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = NodeT;
    using difference_type = ptrdiff_t;
    using pointer = NodeT*;
    using reference = NodeT&;


    /**
     * @brief Constructs an empty LeftEdgeIterator object which can be used as
     * an end iterator.
     */
    LeftEdgeIterator()
        : m_node(nullptr)
    {
    }
    /**
     * @brief Constructs a LeftEdgeIterator which points to the given @a node.
     * @param[in] node The node object where the iterator should point.
     */
    LeftEdgeIterator(ast::ExpressionNode& node)
        : m_node(&node)
    {
    }
    /**
     * @brief Returns a reference to the current node.
     * @return Returns a reference to the current node.
     */
    reference operator*() const
    {
        return *m_node;
    }
    /**
     * @brief Returns a pointer to the current node.
     * @return Returns a pointer to the current node.
     */
    pointer operator->() const
    {
        return m_node;
    }
    /**
     * @brief Checks whether this iterator doesn't equals to the @a other.
     * @param[in] other An iterator object.
     * @return Returns true if other points to a different node than this
     * iterator, otherwise returns false.
     */
    bool operator!=(const LeftEdgeIterator& other) const
    {
        return m_node != other.m_node;
    }
    /**
     * @brief Advances the iterator to the next node on the left edge of the
     * AST.
     * @return Returns an iterator to the next node.
     */
    LeftEdgeIterator& operator++()
    {
        m_node = m_childExtractor(m_node);
        return *this;
    }
    /**
     * @brief Advances the iterator to the next node on the left edge of the
     * AST.
     * @return Returns an iterator to the previous node.
     */
    LeftEdgeIterator operator++(int)
    {
        auto tempIterator = *this;
        m_node = m_childExtractor(m_node);
        return tempIterator;
    }

private:
    /**
     * @brief Pointer to the current node object.
     */
    ast::ExpressionNode* m_node;
    /**
     * @brief Functor used for extracting the left child of the given node.
     */
    LeftChildExtractor m_childExtractor;
};
}} // namespace hermes::path::parser
