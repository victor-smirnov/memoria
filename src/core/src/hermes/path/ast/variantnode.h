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
#ifndef VARIANTNODE_H
#define VARIANTNODE_H
#include "abstractnode.h"
#include "variantvisitoradaptor.h"
#include <boost/variant.hpp>

namespace memoria::hermes::path { namespace ast{

/**
 * @brief The VariantNode class serves as a container node which can represent
 * either one of the node types specified in the classes template argument list.
 * @tparam VariantT The list of types that the VariantNode can represent
 */
template <typename ...VariantT>
class VariantNode : public AbstractNode
{
public:
    /**
     * @brief The internal variant type which stores the nodes defined in
     * VariantT or boost::blank if it's empty.
     */
    using ValueType = boost::variant<boost::blank, VariantT...>;
    /**
     * @brief Constructs an empty VariantNode object.
     */
    VariantNode()
        : AbstractNode()
    {
    }
    /**
     * @brief Copy-constructs an VariantNode object.
     */
    VariantNode(const VariantNode&) = default;
    /**
     * @brief Copy constructs a VariantNode object if T is VariantNode or
     * constructs a VariantNode object with T as the represented node type with
     * the value given in @a other.
     */
    template <typename T>
    VariantNode(const T& other)
        : AbstractNode()
    {
        *this = other;
    }
    /**
     * @brief Assigns the @a other object's value to this object
     * @param[in] other The object whos value should be assigned to this object.
     * @return Returns a reference to this object.
     */
    VariantNode<VariantT...>& operator=(const VariantNode& other)
    {
        if (this != &other)
        {
            value = other.value;
        }
        return *this;
    }
    /**
     * @brief Assigns the value of the @a other object to this object's internal
     * variant making it the node that this object represents.
     */
    template <typename T>
    VariantNode<VariantT...>& operator=(const T& other)
    {
        value = other;
        return *this;
    }
    /**
     * @brief Equality compares this node to the @a other
     * @param[in] other The node that should be compared.
     * @return Returns true if this object is equal to the @a other, otherwise
     * false
     */
    bool operator==(const VariantNode& other) const
    {
        if (this != &other)
        {
            return value == other.value;
        }
        return true;
    }
    /**
     * @brief Returns whether this object has been initialized.
     * @return Returns true if some node's value has been assigned to this
     * object, or false if this object doesn't yet represents any node.
     */
    bool isNull() const
    {
        return value.type() == typeid(boost::blank);
    }

    void accept(interpreter::AbstractVisitor *visitor) const override
    {
        boost::apply_visitor(VariantVisitorAdaptor(visitor), value);
    }
    /**
     * @brief The variable which stores the node that this object represents.
     */
    ValueType value;
};
}} // namespace hermes::path::ast
#endif // VARIANTNODE_H
