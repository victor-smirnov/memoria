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
#include "memoria/core/hermes/path/jmespath.h"
#include "interpreter/interpreter.h"
#include <boost/hana.hpp>

namespace memoria::jmespath {

template <typename JsonT>
std::enable_if_t<std::is_same<std::decay_t<JsonT>, Json>::value, Json>
search(const Expression &expression, JsonT&& document)
{
    using interpreter::Interpreter;
//    using interpreter::JsonRef;

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

    // copy the context value from the interpreter if it's a reference or move
    // it into the local result variable if it's a value, and return the result
    // of the function by value. this approach leaves open the possibility for
    // the compiler to use copy elision to optimize away any further copies or
    // moves
    Json result;
//    auto visitor = boost::hana::overload(
//        [&result](Json& value) mutable {
//            result = std::move(value);
//        }
//    );
//    boost::apply_visitor(visitor, s_interpreter.currentContextValue());
    result = s_interpreter.currentContextValue();
    return result;
}

// explicit instantion
template Json search<const Json&>(const Expression&, const Json&);
template Json search<Json&>(const Expression&, Json&);
template Json search<Json>(const Expression&, Json&&);
} // namespace jmespath
