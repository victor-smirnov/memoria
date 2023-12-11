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

#include <memoria/core/hermes/path/types.h>
#include <memoria/core/hermes/path/exceptions.h>
#include <memoria/core/hermes/path/expression.h>

#include <memoria/core/hermes/traits.hpp>

#include <string>
#include <functional>

namespace memoria {
namespace hermes {
namespace path {

using HermesObjectResolver = std::function<Object(U8StringView)>;

/**
 * @ingroup public
 * @brief Finds or creates the results for the @a expression evaluated on the
 * given @a document.
 *
 * The @a expression string should be encoded in UTF-8.
 * @param expression HermesPath expression.
 * @param document Input JSON document
 * @return Result of the evaluation of the @a expression in @ref ValuePtr format
 * @note This function is reentrant. Since it takes the @a expression by
 * reference the value of the @a expression should be protected from changes
 * until the function returns.
 * @throws InvalidAgrument If a precondition fails. Usually signals an internal
 * error.
 * @throws InvalidValue When an invalid value is specified for an *expression*.
 * For example a `0` step value for a slice expression.
 * @throws UnknownFunction When an unknown HermesPath function is called in the
 * *expression*.
 * @throws InvalidFunctionArgumentArity When a HermesPath function is called with
 * an unexpected number of arguments in the *expression*.
 * @throws InvalidFunctionArgumentType When an invalid type of argument was
 * specified for a HermesPath function call in the *expression*.
 */

MaybeObject search(const Expression& expression, const Object& document);

MaybeObject search(const Expression& expression, const Object& document, const IParameterResolver& resolver);

MaybeObject search(const TinyObjectMap& expression, const Object& document);

MaybeObject search(const TinyObjectMap& expression, const Object& document, const IParameterResolver& resolver);

struct ASTCodes {
    static constexpr NamedCode CODE_ATTR                  = NamedCode(0, "code");

    static constexpr NamedCode AST_NODE_NAME              = NamedCode(1, "astNodeName");
    static constexpr NamedCode IDENTIFIER_NODE            = NamedCode(2, "Identifier");
    static constexpr NamedCode RAW_STRING_NODE            = NamedCode(3, "RawString");
    static constexpr NamedCode HERMES_VALUE_NODE          = NamedCode(4, "HermesValue");
    static constexpr NamedCode SUBEXPRESSION_NODE         = NamedCode(5, "Subexpression");
    static constexpr NamedCode INDEX_EXPRESSION_NODE      = NamedCode(6, "IndexEXpression");
    static constexpr NamedCode ARRAY_ITEM_NODE            = NamedCode(7, "ArrayItem");
    static constexpr NamedCode FLATTEN_OPERATOR_NODE      = NamedCode(8, "FlattenExpression");
    static constexpr NamedCode SLICE_EXPRESSION_NODE      = NamedCode(9, "SliceExpression");
    static constexpr NamedCode LIST_WILDCARD_NODE         = NamedCode(10, "ListWildcard");
    static constexpr NamedCode HASH_WILDCARD_NODE         = NamedCode(11, "HashWildcard");
    static constexpr NamedCode MULTISELECT_LIST_NODE      = NamedCode(12, "MultiselectList");
    static constexpr NamedCode MULTISELECT_HASH_NODE      = NamedCode(13, "MutiselectHash");
    static constexpr NamedCode NOT_EXPRESSION_NODE        = NamedCode(14, "NotExpression");
    static constexpr NamedCode OR_EXPRESSION_NODE         = NamedCode(15, "OrExpression");
    static constexpr NamedCode COMPARATOR_EXPRESSION_NODE = NamedCode(16, "ComparatorExpression");
    static constexpr NamedCode AND_EXPRESSION_NODE        = NamedCode(17, "AndExpression");
    static constexpr NamedCode PAREN_EXPRESSION_NODE      = NamedCode(18, "ParenExpression");
    static constexpr NamedCode CURRENT_NODE               = NamedCode(19, "Current");
    static constexpr NamedCode FILTER_EXPRESSION_NODE     = NamedCode(20, "FilterExpression");
    static constexpr NamedCode PIPE_EXPRESSION_NODE       = NamedCode(21, "PipeExpression");
    static constexpr NamedCode FUNCTION_EXPRESSION_NODE   = NamedCode(22, "FunctionExpression");
    static constexpr NamedCode EXPRESSION_ARGUMENT_NODE   = NamedCode(23, "ExpressionArgument");
    static constexpr NamedCode NULL_NODE                  = NamedCode(24, "NullNode");

    static constexpr NamedCode LEFT_EXPRESSION_ATTR   = NamedCode(25, "leftExpression");
    static constexpr NamedCode RIGHT_EXPRESSION_ATTR  = NamedCode(26, "rightExpression");
    static constexpr NamedCode EXPRESSION_ATTR        = NamedCode(27, "expression");
    static constexpr NamedCode EXPRESSIONS_ATTR       = NamedCode(28, "expressions");
    static constexpr NamedCode IDENTIFIER_ATTR        = NamedCode(29, "identifier");
    static constexpr NamedCode RAW_STRING_ATTR        = NamedCode(30, "rawString");
    static constexpr NamedCode VALUE_ATTR             = NamedCode(31, "value");
    static constexpr NamedCode BRACKET_SPECIFIER_ATTR = NamedCode(32, "bracketSpecifier");
    static constexpr NamedCode IS_PROJECTION_ATTR     = NamedCode(33, "isProjection");
    static constexpr NamedCode STOPS_PROJECTION_ATTR  = NamedCode(34, "stopsProjection");
    static constexpr NamedCode INDEX_ATTR             = NamedCode(35, "index");
    static constexpr NamedCode STEP_ATTR              = NamedCode(36, "step");
    static constexpr NamedCode START_ATTR             = NamedCode(37, "start");
    static constexpr NamedCode STOP_ATTR              = NamedCode(38, "stop");
    static constexpr NamedCode FIRST_ATTR             = NamedCode(39, "first");
    static constexpr NamedCode SECOND_ATTR            = NamedCode(40, "second");
    static constexpr NamedCode COMPARATOR_ATTR        = NamedCode(41, "comparator");
    static constexpr NamedCode FUNCTION_NAME_ATTR     = NamedCode(42, "functionName");
    static constexpr NamedCode ARGUMENTS_ATTR         = NamedCode(43, "arguments");
};


}
}}
