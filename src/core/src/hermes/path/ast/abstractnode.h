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
#ifndef ABSTRACTNODE_H
#define ABSTRACTNODE_H

namespace memoria::jmespath { namespace interpreter {

class AbstractVisitor;
}}

/**
 * @namespace jmespath::ast
 * @brief Classes which represent the AST nodes
 */
namespace memoria::jmespath { namespace ast {
/**
 * @brief The AbstractNode class is the common interface class
 * for all AST node types
 */
class AbstractNode
{
public:
    /**
     * @brief Constructs an AbstractNode object.
     */
    AbstractNode() = default;
    /**
     * @brief Copy-constructs an AbstractNode object.
     */
    AbstractNode(const AbstractNode&) = default;
    /**
     * @brief Destroys the AbstractNode object.
     */
    virtual ~AbstractNode();
    /**
     * @brief Copy-assigns the other object to this object.
     * @return Returns a reference to this object.
     */
    AbstractNode& operator=(const AbstractNode&) = default;
    /**
     * @brief Accepts the given @a visitor object.
     *
     * Subclasses should implement this function by calling the visit
     * method of the @a visitor with a pointer to the node object itself
     * and the accept method of the node's member nodes with the @a visitor as
     * the parameter.
     * @param[in] visitor An @ref interpreter::AbstractVisitor object
     */
    virtual void accept(interpreter::AbstractVisitor* visitor) const = 0;
};
}} // namespace jmespath::ast
#endif // ABSTRACTNODE_H
