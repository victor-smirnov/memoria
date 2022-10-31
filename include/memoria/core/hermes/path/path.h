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

#include <string>
#include <memoria/core/hermes/path/types.h>
#include <memoria/core/hermes/path/exceptions.h>
#include <memoria/core/hermes/path/expression.h>

namespace memoria {
namespace hermes {
namespace path {

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

ValuePtr search(const Expression& expression, const ValuePtr& document);

ValuePtr search(const Expression& expression, const ValuePtr& document, const IParameterResolver& resolver);

}
}}
