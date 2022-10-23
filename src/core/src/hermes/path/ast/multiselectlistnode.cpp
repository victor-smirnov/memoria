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
#include "multiselectlistnode.h"
#include "allnodes.h"

namespace memoria::hermes::path { namespace ast {

MultiselectListNode::MultiselectListNode()
    : AbstractNode()
{
}

MultiselectListNode::MultiselectListNode(
        const std::vector<ExpressionNode> &subexpressions)
    : AbstractNode(),
      expressions(subexpressions)
{
}

MultiselectListNode::MultiselectListNode(
        const std::initializer_list<ExpressionNode> &subexpressions)
    : AbstractNode(),
      expressions(subexpressions)
{
}

void MultiselectListNode::accept(interpreter::AbstractVisitor *visitor) const
{
    visitor->visit(this);
}

bool MultiselectListNode::operator==(const MultiselectListNode &other) const
{
    if (this != &other)
    {
        return expressions == other.expressions;
    }
    return true;
}
}} // namespace hermes::path::ast
