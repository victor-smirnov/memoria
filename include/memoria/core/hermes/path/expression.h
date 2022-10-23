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

#include <memory>
#include <memoria/core/hermes/path/types.h>
#include <memoria/core/hermes/path/exceptions.h>

namespace memoria {
namespace hermes {
namespace path {

namespace ast {
class ExpressionNode;
}
/**
 * @ingroup public
 * @brief The Expression class represents a HermesPath expression.
 *
 * The Expression class can be used to store a parsed HermesPath expression and
 * reuse it for multiple searches.
 * @note This class is reentrant.
 */
class Expression
{
public:
    /**
     * @brief Constructs an empty Expression object.
     */
    Expression();
    /**
     * @brief Constructs a copy of @a other.
     * @param[in] other The object that should be copied.
     */
    Expression(const Expression& other);
    /**
     * @brief Move-constructs an Expression by moving the value of @a other to
     * this object.
     * @param[in] other The object whose value should be mvoed.
     */
    Expression(Expression&& other);
    /**
     * @brief Constructs an Expression object by forwarding @a argument.
     *
     * This constructor participates in overload resolution only if U is
     * implicitly convertible to String. @a Argument should describe a
     * valid HermesPath expression.
     * @param[in] argument The value that should be forwarded.
     * @tparam U The type of @a argument.
     * @throws SyntaxError When the syntax of the specified *expression* is
     * invalid.
     */
    template <typename U, typename
        std::enable_if<
            std::is_convertible<U, String>::value>::type* = nullptr>
    Expression(U&& expression)
        : m_expressionString(std::forward<U>(expression))
    {
        parseExpression(m_expressionString);
    }
    /**
     * @brief Assigns @a other to this expression and returns a reference to
     * this expression.
     * @param[in] other The expression that should be assigned.
     * @return Reference to this expression.
     */
    Expression& operator= (const Expression& other);
    /**
     * @brief Move-assigns @a other to this expression and returns a reference
     * to this expression.
     * @param[in] other The expression that should be moved.
     * @return Reference to this expression.
     */
    Expression& operator= (Expression&& other);
    /**
     * @brief Equality compares this expression to the @a other.
     * @param[in] other The expression that should be compared.
     * @return Returns true if this object is equal to the @a other, otherwise
     * false
     */
    bool operator== (const Expression& other) const;
    /**
     * @brief Converts the expression to the string representation of the
     * HermesPath expression.
     * @return[in] String representation of the HermesPath expression.
     */
    String toString() const;
    /**
     * @brief Checks whether this object has been initialized.
     * expression.
     * @return Returns true the expression has not been initialized yet,
     * otherwise returns false.
     */
    bool isEmpty() const;
    /**
     * @brief Returns a pointer to the root expression in the abstract syntax
     * tree.
     * @return A pointer to the root expression or `nullptr` if the object is
     * empty.
     */
    const ast::ExpressionNode* astRoot() const;

private:
    /**
     * @brief The ExpressionDeleter struct is a custom destruction policy
     * for deleting ast::ExpressionNode objects.
     *
     * Unlike std::default_deleter it can be used to delete forward declared
     * @ref ast::ExpressionNode.
     */
    struct ExpressionDeleter
    {
        /**
         * @brief operator () Destroys the given @a node object.
         * @param node An instance of ast::ExpressionNode
         */
        void operator()(ast::ExpressionNode* node) const;
    };
    /**
     * @brief The string representation of the HermesPath expression.
     */
    String m_expressionString;
    /**
     * @brief The root node of the ast.
     */
    std::unique_ptr<ast::ExpressionNode, ExpressionDeleter> m_astRoot;
    /**
     * @brief Parses the @a expressionString and updates the AST.
     * @param[in] expressionString The string representation of the HermesPath
     * expression.
     * @throws SyntaxError When the syntax of the specified
     * *expressionString* is invalid.
     */
    void parseExpression(const String &expressionString);
};

/**
 * @brief User defined literals
 */
namespace literals {

/**
 * @brief User defined string literal for HermesPath expressions
 *
 * This operator implements a user defined string literal for HermesPath
 * expressions. It can be used by appending `"_jmespath"` to a string literal.
 * @param[in] expression The string representation of a HermesPath expression.
 * @return An Expression object.
 * @throws SyntaxError When the syntax of the specified *expression* is invalid.
 */
inline Expression operator""_hermespath(const char* expression, std::size_t)
{
    return {expression};
}

}
}
}}
