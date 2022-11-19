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

#pragma once

#include <memoria/core/strings/u8_string.hpp>

#include "abstractnode.h"
#include "variantnode.h"
#include <boost/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>



namespace memoria::hermes::path { namespace ast {

class IdentifierNode;
class RawStringNode;
class SubexpressionNode;
class IndexExpressionNode;
class HashWildcardNode;
class MultiselectListNode;
class MultiselectHashNode;
class NotExpressionNode;
class ComparatorExpressionNode;
class OrExpressionNode;
class AndExpressionNode;
class ParenExpressionNode;
class PipeExpressionNode;
class CurrentNode;
class FunctionExpressionNode;
class HermesValueNode;

struct ExpressionAttrs {
    static constexpr U8StringView CODE_ATTR         = "code";
    static constexpr U8StringView LEFT_EXPRESSION   = "leftExpression";
    static constexpr U8StringView RIGHT_EXPRESSION  = "rightExpression";
    static constexpr U8StringView EXPRESSION        = "expression";
    static constexpr U8StringView EXPRESSIONS       = "expressions";
    static constexpr U8StringView IDENTIFIER        = "identifier";
    static constexpr U8StringView RAW_STRING        = "rawString";
    static constexpr U8StringView VALUE             = "value";
    static constexpr U8StringView BRACKET_SPECIFIER = "bracketSpecifier";
    static constexpr U8StringView IS_PROJECTION     = "isProjection";
    static constexpr U8StringView STOPS_PROJECTION  = "stopsProjection";
    static constexpr U8StringView INDEX             = "index";
    static constexpr U8StringView STEP              = "step";
    static constexpr U8StringView START             = "start";
    static constexpr U8StringView STOP              = "stop";
    static constexpr U8StringView FIRST             = "first";
    static constexpr U8StringView SECOND            = "second";
    static constexpr U8StringView COMPARATOR        = "comparator";
    static constexpr U8StringView FUNCTION_NAME     = "functionName";
    static constexpr U8StringView ARGUMENTS         = "arguments";
};

/**
 * @brief The ExpressionNode class represents a HermesPath expression.
 */
class ExpressionNode : public VariantNode<
        boost::recursive_wrapper<IdentifierNode>,
        boost::recursive_wrapper<RawStringNode>,
        boost::recursive_wrapper<HermesValueNode>,
        boost::recursive_wrapper<SubexpressionNode>,
        boost::recursive_wrapper<IndexExpressionNode>,
        boost::recursive_wrapper<HashWildcardNode>,
        boost::recursive_wrapper<MultiselectListNode>,
        boost::recursive_wrapper<MultiselectHashNode>,
        boost::recursive_wrapper<NotExpressionNode>,
        boost::recursive_wrapper<ComparatorExpressionNode>,
        boost::recursive_wrapper<OrExpressionNode>,
        boost::recursive_wrapper<AndExpressionNode>,
        boost::recursive_wrapper<ParenExpressionNode>,
        boost::recursive_wrapper<PipeExpressionNode>,
        boost::recursive_wrapper<CurrentNode>,
        boost::recursive_wrapper<FunctionExpressionNode> >
{
public:
    static constexpr int64_t NULL_NODE_CODE = 10000;

    /**
     * @brief Constructs an empty ExpressionNode object
     */
    ExpressionNode();
    /**
     * @brief Copy-constructs an ExpressionNode object.
     */
    ExpressionNode(const ExpressionNode&) = default;
    /**
     * @brief Destroys the ExpressionNode object.
     */
    virtual ~ExpressionNode();
    /**
     * @brief Constructs an ExpressionNode object with its child expression
     * initialized to @a expression
     * @param[in] expression The node's child expression
     */
    ExpressionNode(const ValueType& expression);
    /**
     * @brief Assigns the @a other object to this object.
     * @param[in] other An ExpressionNode object.
     * @return Returns a reference to this object.
     */
    ExpressionNode& operator=(const ExpressionNode& other);
    /**
     * @brief Assigns the @a other Expression to this object's expression.
     * @param[in] expression An Expression object.
     * @return Returns a reference to this object.
     */
    ExpressionNode& operator=(const ValueType& expression);
};
}} // namespace hermes::path::ast

BOOST_FUSION_ADAPT_STRUCT(
    memoria::hermes::path::ast::ExpressionNode,
    (memoria::hermes::path::ast::ExpressionNode::ValueType, value)
)
