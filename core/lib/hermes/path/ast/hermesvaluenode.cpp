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
#include "hermesvaluenode.h"
#include "../interpreter/abstractvisitor.h"

namespace memoria::hermes::path { namespace ast {

HermesValueNode::HermesValueNode()
    : AbstractNode()
{
}

HermesValueNode::HermesValueNode(const Object &value)
    : AbstractNode(),
      value(value)
{
}

void HermesValueNode::accept(interpreter::AbstractVisitor *visitor) const
{
    visitor->visit(this);
}

bool HermesValueNode::operator==(const HermesValueNode &other) const
{
    if (this != &other)
    {
        return value.equals(other.value);
    }
    return true;
}

Object HermesArrayNode::to_hermes_array(HermesCtrView& doc) const
{
    auto array = doc.make_object_array();

    for (auto& var: this->array) {
        array.push_back(std::move(var.value));
    }

    return array.as_object();
}



}} // namespace hermes::path::ast
