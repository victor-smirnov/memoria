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
#ifndef VARIANTVISITOR_H
#define VARIANTVISITOR_H
#include "../interpreter/abstractvisitor.h"
#include <boost/variant.hpp>

namespace memoria::jmespath { namespace ast {

/**
 * @brief The VariantVisitorAdaptor class adapts an AbstractVisitor
 * implementation to the boost::static_visitor interface, so it can be used
 * to visit boost::variant objects.
 */
class VariantVisitorAdaptor : public boost::static_visitor<>
{
public:
    /**
     * @brief Constructs a VariantVisitorAdaptor object with the given
     * @a visitor
     * @param[in] visitor The visitor object to which the visit calls will be
     * forwarded.
     */
    VariantVisitorAdaptor(interpreter::AbstractVisitor* visitor);
    /**
     * @brief Calls the appropriate visit method of the visitor object with the
     * address of the @a variant object.
     * @param[in] variant The object that the visitor should visit.
     */
    template <typename T>
    void operator() (const T& variant) const
    {
        m_visitor->visit(&variant);
    }
    /**
     * @brief Does nothing, defined to ignore empty variants and to avoid
     * calling the visitor object with a blank value.
     */
    void operator() (const boost::blank&) const
    {
    }

private:
    /**
     * @brief The visitor object to which the visit calls will be forwarded.
     */
    interpreter::AbstractVisitor* m_visitor;
};
}} // namespace jmespath::ast
#endif // VARIANTVISITOR_H
