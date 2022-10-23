/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
**
** This file is based on the jmespath.cpp project
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
#include "memoria/core/hermes/path/jmespath.h"
#include "interpreter/interpreter.h"
#include <boost/hana.hpp>

namespace memoria::hermes::path {

template <typename JsonT>
std::enable_if_t<std::is_same<std::decay_t<JsonT>, ValuePtr>::value, ValuePtr>
search(const Expression &expression, JsonT&& document)
{
    using interpreter::Interpreter;
    if (expression.isEmpty())
    {
        return {};
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
    thread_local Interpreter s_interpreter;
#pragma clang diagnostic pop
    s_interpreter.setContext(std::forward<JsonT>(document));
    // evaluate the expression by calling visit with the root of the AST
    s_interpreter.visit(expression.astRoot());

    return s_interpreter.currentContextValue();
}

// explicit instantion
template ValuePtr search<const ValuePtr&>(const Expression&, const ValuePtr&);
template ValuePtr search<ValuePtr&>(const Expression&, ValuePtr&);
template ValuePtr search<ValuePtr>(const Expression&, ValuePtr&&);
} // namespace hermes::path
