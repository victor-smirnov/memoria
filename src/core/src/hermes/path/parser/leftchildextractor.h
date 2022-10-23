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
#ifndef LEFTCHILDEXTRACTOR_H
#define LEFTCHILDEXTRACTOR_H
#include "../ast/binaryexpressionnode.h"
#include "../ast/notexpressionnode.h"

namespace memoria::hermes::path { namespace parser {

/**
 * @brief The LeftChildExtractor class is a functor that extracts the
 * child expression node from the given @a node which should be evaluated
 * before the given @a node itself.
 */
class LeftChildExtractor
        : public boost::static_visitor<ast::ExpressionNode*>
{
public:
    /**
     * @brief Returns a pointer to the child expression node of the given
     * @a node or nullptr if the @a node doesn't has a suitable child node.
     * @param[in] node The node whose child node should be extracted.
     * @{
     */
    ast::ExpressionNode* operator()(ast::ExpressionNode* node) const
    {
        return boost::apply_visitor(*this, node->value);
    }

    template <typename T>
    ast::ExpressionNode* operator()(T& node) const
    {
        return childNode(&node);
    }

    template <typename T, typename
        std::enable_if<
            !std::is_base_of<ast::BinaryExpressionNode,
                             T>::value
            && !std::is_same<ast::NotExpressionNode,
                             T>::value, int>::type = 0>
    ast::ExpressionNode* childNode(T*) const
    {
        return nullptr;
    }

    template <typename T, typename
        std::enable_if<
            !std::is_base_of<ast::BinaryExpressionNode,
                             T>::value
            && std::is_same<ast::NotExpressionNode,
                            T>::value, int>::type = 0>
    ast::ExpressionNode* childNode(T* node) const
    {
        return &node->expression;
    }

    template <typename T, typename
        std::enable_if<
            std::is_base_of<ast::BinaryExpressionNode,
                            T>::value, int>::type = 0>
    ast::ExpressionNode* childNode(T* node) const
    {
        return &node->leftExpression;
    }    
    /** @}*/
};
}} // namespace hermes::path::parser
#endif // LEFTCHILDEXTRACTOR_H
