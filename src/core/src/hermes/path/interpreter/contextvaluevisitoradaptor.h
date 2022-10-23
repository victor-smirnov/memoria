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
#ifndef CONTEXTVALUEVISITORADAPTOR_H
#define CONTEXTVALUEVISITORADAPTOR_H
#include "interpreter.h"
#include "memoria/core/hermes/path/exceptions.h"
#include <boost/variant.hpp>
#include <boost/hana.hpp>

namespace memoria::hermes::path { namespace interpreter {

/**
 * @brief The ContextValueVisitorAdaptor class adapts a visitor object,
 * which is callable with const lvalue reference of @ref ValuePtr and with rvalue
 * reference of @ref ValuePtr objects, to @ref ContextValue objects.
 */
template <typename VisitorT, bool ForceMove = false>
class ContextValueVisitorAdaptor : public boost::static_visitor<>
{
public:
    /**
     * @brief Constructs a ContextValueVisitorAdaptor object, adapting the
     * @a visitor object to be able to consume @ref ContextValue objects.
     * @param[in] visitor A visitor which is callable with const lvalue
     * reference of @ref ValuePtr and with rvalue reference of @ref ValuePtr objects.
     */
    ContextValueVisitorAdaptor(VisitorT&& visitor)
        : boost::static_visitor<>{},
          m_visitor{std::move(visitor)}
    {
    }

    /**
     * @brief Calls the visitor object with the rvalue reference of @a value.
     * @param[in] value A @ref ValuePtr value.
     */
    void operator()(ValuePtr& value)
    {
        m_visitor(std::move(value));
    }

private:
    /**
     * @brief The visitor object to which the calls will be forwarded.
     */
    VisitorT m_visitor;
};

/**
 * @brief Create visitor object which accepts @ref ContextValue
 * objects, and calls @a lvalueFunc callable with a const lvalue ref of the
 * @ref ValuePtr reference held by the @ref ContextValue or calls the @a rvalueFunc
 * callable with an rvalue ref of the @ref ValuePtr object held by
 * @ref ContextValue.
 * @param[in] lvalueFunc A callable taking a const lvalue reference to ValuePtr.
 * @param[in] rvalueFunc A callable taking an rvalue reference to ValuePtr.
 * @return A visitor object which accepts @ref ContextValue objects
 */
inline decltype(auto) makeVisitor(
    std::function<void(const ValuePtr&)> &&lvalueFunc,
    std::function<void(ValuePtr&&)> &&rvalueFunc)
{
    auto functions = boost::hana::overload_linearly(
        std::move(rvalueFunc),
        std::move(lvalueFunc)
    );
    return ContextValueVisitorAdaptor<decltype(functions)>{
        std::move(functions)
    };
}

/**
 * @brief Create visitor object which accepts @ref ContextValue
 * objects, and calls the @a rvalueFunc callable with an rvalue ref of the
 * @ref ValuePtr object held by the @ref ContextValue or calls it with the rvalue
 * ref of the copy of the object to which the @ref ValuePtr reference points in the
 * @ref ContextValue object.
 * @param[in] rvalueFunc A callable taking an rvalue reference to @ref ValuePtr.
 * @return A visitor object which accepts @ref ContextValue objects
 */
inline decltype(auto) makeMoveOnlyVisitor(
        std::function<void(ValuePtr&&)> rvalueFunc)
{
    auto result = ContextValueVisitorAdaptor<decltype(rvalueFunc), true>{
        std::move(rvalueFunc)
    };
    return result;
}
}} // namespace hermes::path::interpreter
#endif // CONTEXTVALUEVISITORADAPTOR_H
