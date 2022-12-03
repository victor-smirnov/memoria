/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
** Copyright (c) 2022 Victor Smirnov
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

#include "abstractnode.h"
#include <memoria/core/hermes/path/types.h>
#include <boost/fusion/include/adapt_struct.hpp>

#include <memoria/core/hermes/hermes.hpp>

#include <vector>

namespace memoria::hermes::path { namespace ast {

/**
 * @brief The HermesValueNode class represents a Hermes Object.
 */
class HermesValueNode : public AbstractNode
{
public:
    /**
     * @brief Constructs an empty RawStringNode object.
     */
    HermesValueNode();
    /**
     * @brief Constructs a RawStringNode object with its value initialized to
     * @a string
     * @param[in] string The raw string value.
     */
    HermesValueNode(const ObjectPtr& string);
    /**
     * @brief Calls the visit method of the given @a visitor with the
     * dynamic type of the node.
     * @param[in] visitor A visitor implementation
     */
    void accept(interpreter::AbstractVisitor* visitor) const override;
    /**
     * @brief Equality compares this node to the @a other
     * @param[in] other The node that should be compared.
     * @return Returns true if this object is equal to the @a other, otherwise
     * false
     */
    bool operator==(const HermesValueNode& other) const;

    operator ObjectPtr() const {
        return value;
    }

    /**
     * @brief The raw Hermes value
     */
    ObjectPtr value;

    static constexpr NamedCode CODE = ASTCodes::HERMES_VALUE_NODE;
};


struct HermesArrayNode
{
    std::vector<HermesValueNode> array;
    ObjectPtr to_hermes_array(HermesCtr& doc) const;
};




}} // namespace hermes::path::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::HermesValueNode,
    (memoria::hermes::ObjectPtr, value)
)

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::HermesArrayNode,
    (std::vector<memoria::hermes::path::ast::HermesValueNode>, array)
)
